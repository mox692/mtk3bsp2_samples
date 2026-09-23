/*
 * stm32h7_fdcan.c - Hardware backend for the STM32H7 FDCAN (e.g. H723).
 *
 * This is the physical layer for the STM32H7 family.  It shares the public
 * API and the can_hw_ops_t contract with the STM32H5 backend, but the two
 * controllers differ in ways that must stay hidden behind this file:
 *
 *   - Message RAM is SOFTWARE-PLACED here: the driver picks the start offset
 *     and element count of the RX FIFO / TX FIFO and programs them through
 *     RXF0C / TXBC (the H5 has a hard-wired, fixed layout instead).
 *   - Each element is 16 bytes (8-byte header + 8-byte data) rather than the
 *     72-byte fixed element of the H5.
 *   - Several register offsets differ (TXBAR at 0x0D0, RX FIFO0 status/ack at
 *     0x0A4/0x0A8), and the FIFO index/fill-level fields are wider.
 *   - The FDCAN clock calibration unit (CCU) must be told to bypass calibration
 *     and use the kernel clock directly (BCC bit).
 *
 * GPIO, the peripheral/kernel clock and the transceiver are the board's job
 * (see the H723 Application/can_board.c); this file only touches FDCAN and its
 * message RAM through dev->reg_base / dev->msgram_base.
 */
#include <can/can.h>
#include "../internal/can_hw.h"

/* ------------------------------------------------------------------------ */
/*  Register access helpers (no BSP / uT-Kernel dependency)                 */
/* ------------------------------------------------------------------------ */
static inline void reg_write(uint32_t addr, uint32_t val)
{
	*(volatile uint32_t *)addr = val;
}

static inline uint32_t reg_read(uint32_t addr)
{
	return *(volatile uint32_t *)addr;
}

/* --- FDCAN register offsets from the instance base (RM0468 ch.61) --- */
#define FDCAN_CCCR_OFF		0x018u
#define FDCAN_NBTP_OFF		0x01Cu
#define FDCAN_ECR_OFF		0x040u	/* error counters   */
#define FDCAN_PSR_OFF		0x044u	/* protocol status  */
#define FDCAN_IR_OFF		0x050u	/* interrupt reg    */
#define FDCAN_GFC_OFF		0x080u	/* global filter cfg*/
#define FDCAN_SIDFC_OFF		0x084u	/* std ID filter cfg*/
#define FDCAN_RXF0C_OFF		0x0A0u	/* RX FIFO0 config  */
#define FDCAN_RXF0S_OFF		0x0A4u	/* RX FIFO0 status  */
#define FDCAN_RXF0A_OFF		0x0A8u	/* RX FIFO0 ack     */
#define FDCAN_RXESC_OFF		0x0BCu	/* RX element size  */
#define FDCAN_TXBC_OFF		0x0C0u	/* TX buffer config */
#define FDCAN_TXFQS_OFF		0x0C4u	/* TX FIFO/Q status */
#define FDCAN_TXESC_OFF		0x0C8u	/* TX element size  */
#define FDCAN_TXBAR_OFF		0x0D0u	/* TX buffer add req*/

#define FDCAN_CCCR_INIT		(1u << 0)
#define FDCAN_CCCR_CCE		(1u << 1)
#define FDCAN_CCCR_DAR		(1u << 6)	/* disable auto retransmission */
#define FDCAN_CCCR_MON		(1u << 5)	/* bus monitoring              */
#define FDCAN_CCCR_TEST		(1u << 7)	/* test mode enable            */
#define FDCAN_CCCR_FDOE		(1u << 8)	/* FD operation enable         */
#define FDCAN_CCCR_BRSE		(1u << 9)	/* bit rate switching          */

#define FDCAN_TEST_OFF		0x010u
#define FDCAN_TEST_LBCK		(1u << 4)	/* loop back mode              */

#define FDCAN_IR_RF0L		(1u << 3)	/* RX FIFO0 message lost       */

#define FDCAN_TXFQS_TFQF	(1u << 21)	/* TX FIFO/Queue full          */
#define FDCAN_TXFQS_TFFL_MASK	0x3Fu		/* free level (bits[5:0])      */
#define FDCAN_TXFQS_TFQPI_SH	16u
#define FDCAN_TXFQS_TFQPI_MASK	0x1Fu		/* put index (up to 32 elems)  */

#define FDCAN_RXF0S_F0FL_MASK	0x7Fu		/* fill level                  */
#define FDCAN_RXF0S_F0GI_SH	8u
#define FDCAN_RXF0S_F0GI_MASK	0x3Fu		/* get index (up to 64 elems)  */

#define FDCAN_PSR_LEC_MASK	0x7u
#define FDCAN_PSR_EP		(1u << 5)
#define FDCAN_PSR_BO		(1u << 7)

/*
 * FDCAN clock calibration unit (CCU).  Shared across all FDCAN instances on
 * the STM32H7 and located at a fixed address, so it is not derived from
 * dev->reg_base.  BCC bypasses calibration and clocks FDCAN from the kernel
 * clock directly - without it the controller will not leave initialization.
 */
#define FDCAN_CCU_CCFG		0x4000A804u
#define FDCANCCU_CCFG_BCC	(1u << 6)

/* --- Software-placed message RAM layout (byte offsets from msgram base) --- */
/*  Classic CAN, 8-byte payload => 8-byte header + 8-byte data = 16 B/element. */
/*    Std ID filter : 0x000-0x003  (1 element  x  4B  =   4 B)                 */
/*    RX FIFO0      : 0x040-0x13F  (16 elements x 16B  = 256 B)                */
/*    TX FIFO       : 0x140-0x1BF  ( 8 elements x 16B  = 128 B)                */
/*  Ends at 0x1C0 (448B), well within the shared message RAM.  If a second    */
/*  FDCAN instance is ever enabled, its regions must not overlap these.       */
#define CAN_RX_FIFO_SIZE	16u	/* # of RX FIFO0 elements  */
#define CAN_TX_FIFO_SIZE	8u	/* # of TX FIFO elements   */
#define CAN_ELEMENT_SIZE	16u	/* header(8) + data(8) bytes/element */

#define SRAMCAN_RXF0_OFFSET	0x040u	/* RX FIFO0 start offset */
#define SRAMCAN_TXB_OFFSET	(SRAMCAN_RXF0_OFFSET + \
				 CAN_RX_FIFO_SIZE * CAN_ELEMENT_SIZE)

/* Bounded wait for INIT bit changes so a stuck controller can't hang us. */
#define CAN_INIT_TIMEOUT	100000u

/* ------------------------------------------------------------------------ */
static can_result_t h7_init(can_device_t *dev, const can_bit_timing_t *t,
			    const can_config_t *config)
{
	uint32_t base = dev->reg_base;
	uint32_t cccr;
	uint32_t nbtp;
	uint32_t spin;

	/* Enter INIT and enable configuration change. */
	reg_write(base + FDCAN_CCCR_OFF,
		  reg_read(base + FDCAN_CCCR_OFF) | FDCAN_CCCR_INIT);
	for (spin = 0u; (reg_read(base + FDCAN_CCCR_OFF) & FDCAN_CCCR_INIT) == 0u;
	     spin++) {
		if (spin >= CAN_INIT_TIMEOUT) {
			return CAN_ERR_TIMEOUT;
		}
	}
	reg_write(base + FDCAN_CCCR_OFF,
		  reg_read(base + FDCAN_CCCR_OFF) | FDCAN_CCCR_CCE);

	/* Classic CAN only: clear FD operation and bit-rate switching. */
	cccr = reg_read(base + FDCAN_CCCR_OFF);
	cccr &= ~(FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE);

	/* Auto retransmission: DAR set => disabled (one-shot). */
	if (config->auto_retransmission) {
		cccr &= ~FDCAN_CCCR_DAR;
	} else {
		cccr |= FDCAN_CCCR_DAR;
	}

	/* Internal loopback: enable test mode + monitor so nothing goes on-bus. */
	if (config->internal_loopback) {
		cccr |= (FDCAN_CCCR_TEST | FDCAN_CCCR_MON);
	} else {
		cccr &= ~(FDCAN_CCCR_TEST | FDCAN_CCCR_MON);
	}
	reg_write(base + FDCAN_CCCR_OFF, cccr);

	if (config->internal_loopback) {
		reg_write(base + FDCAN_TEST_OFF,
			  reg_read(base + FDCAN_TEST_OFF) | FDCAN_TEST_LBCK);
	}

	/* Bypass the clock calibration unit (clock FDCAN from the kernel clock). */
	reg_write(FDCAN_CCU_CCFG,
		  reg_read(FDCAN_CCU_CCFG) | FDCANCCU_CCFG_BCC);

	/*
	 * Bit timing.  NBTP encodes the "minus one" quanta counts:
	 *   NBTP = (NSJW-1)<<25 | (NBRP-1)<<16 | (NTSEG1-1)<<8 | (NTSEG2-1)<<0
	 */
	nbtp = ((t->sjw   - 1u) << 25) |
	       ((t->brp   - 1u) << 16) |
	       ((t->tseg1 - 1u) << 8)  |
	       ((t->tseg2 - 1u) << 0);
	reg_write(base + FDCAN_NBTP_OFF, nbtp);

	/*
	 * Accept every frame into RX FIFO0.  GFC=0 means: non-matching std/ext
	 * frames go to FIFO0, remote frames accepted.  No std ID filter list.
	 */
	reg_write(base + FDCAN_GFC_OFF, 0u);
	reg_write(base + FDCAN_SIDFC_OFF, 0u);

	/*
	 * RX FIFO0: place it in message RAM and select the 8-byte data section.
	 *   RXF0C: F0S (size) bits[22:16], F0SA (start addr) bits[15:2].
	 *   RXESC: F0DS=0 -> 8-byte data field.
	 */
	reg_write(base + FDCAN_RXF0C_OFF,
		  (CAN_RX_FIFO_SIZE << 16) | SRAMCAN_RXF0_OFFSET);
	reg_write(base + FDCAN_RXESC_OFF, 0u);

	/*
	 * TX FIFO: place it in message RAM and select the 8-byte data section.
	 *   TXBC: TFQS (FIFO/queue size) bits[29:24], TBSA (start) bits[15:2],
	 *         TFQM (bit30)=0 -> FIFO mode.  The size MUST go in TFQS.
	 *   TXESC: TBDS=0 -> 8-byte data field.
	 */
	reg_write(base + FDCAN_TXBC_OFF,
		  (CAN_TX_FIFO_SIZE << 24) | SRAMCAN_TXB_OFFSET);
	reg_write(base + FDCAN_TXESC_OFF, 0u);

	return CAN_OK;
}

static can_result_t h7_start(can_device_t *dev)
{
	uint32_t base = dev->reg_base;
	uint32_t spin;

	/* Leave CCE + INIT -> normal mode. */
	reg_write(base + FDCAN_CCCR_OFF,
		  reg_read(base + FDCAN_CCCR_OFF) & ~(FDCAN_CCCR_CCE | FDCAN_CCCR_INIT));
	for (spin = 0u; (reg_read(base + FDCAN_CCCR_OFF) & FDCAN_CCCR_INIT) != 0u;
	     spin++) {
		if (spin >= CAN_INIT_TIMEOUT) {
			return CAN_ERR_TIMEOUT;
		}
	}
	return CAN_OK;
}

static can_result_t h7_stop(can_device_t *dev)
{
	uint32_t base = dev->reg_base;
	uint32_t spin;

	reg_write(base + FDCAN_CCCR_OFF,
		  reg_read(base + FDCAN_CCCR_OFF) | FDCAN_CCCR_INIT);
	for (spin = 0u; (reg_read(base + FDCAN_CCCR_OFF) & FDCAN_CCCR_INIT) == 0u;
	     spin++) {
		if (spin >= CAN_INIT_TIMEOUT) {
			return CAN_ERR_TIMEOUT;
		}
	}
	return CAN_OK;
}

static can_result_t h7_try_send(can_device_t *dev, const can_frame_t *frame)
{
	uint32_t base = dev->reg_base;
	uint32_t txfqs = reg_read(base + FDCAN_TXFQS_OFF);
	uint32_t pidx;
	uint32_t elem;
	uint32_t t1;
	uint32_t w0 = 0u, w1 = 0u;
	uint32_t i;

	/* Refuse if the controller is bus-off. */
	if ((reg_read(base + FDCAN_PSR_OFF) & FDCAN_PSR_BO) != 0u) {
		return CAN_ERR_BUS_OFF;
	}
	/* TX FIFO full: TFQF set or no free level.  Writing anyway would clobber
	 * a pending frame, a likely cause of "TX sometimes stops" symptoms. */
	if ((txfqs & FDCAN_TXFQS_TFQF) != 0u) {
		return CAN_ERR_BUSY;
	}
	if ((txfqs & FDCAN_TXFQS_TFFL_MASK) == 0u) {
		return CAN_ERR_BUSY;
	}

	pidx = (txfqs >> FDCAN_TXFQS_TFQPI_SH) & FDCAN_TXFQS_TFQPI_MASK;
	elem = dev->msgram_base + SRAMCAN_TXB_OFFSET + pidx * CAN_ELEMENT_SIZE;

	/* T0: standard ID left-aligned in bits[28:18]; XTD=0. RTR in bit29. */
	reg_write(elem + 0x00u,
		  ((frame->id & 0x7FFu) << 18) |
		  (frame->remote ? (1u << 29) : 0u));
	/* T1: DLC in bits[19:16]; Classic CAN (FDF=0, BRS=0). */
	t1 = ((uint32_t)frame->len << 16);
	reg_write(elem + 0x04u, t1);

	for (i = 0u; i < frame->len; i++) {
		if (i < 4u) {
			w0 |= ((uint32_t)frame->data[i]) << (8u * i);
		} else {
			w1 |= ((uint32_t)frame->data[i]) << (8u * (i - 4u));
		}
	}
	reg_write(elem + 0x08u, w0);
	reg_write(elem + 0x0Cu, w1);

	/* Request transmission of this buffer. */
	reg_write(base + FDCAN_TXBAR_OFF, (1u << pidx));

	return CAN_OK;
}

static can_result_t h7_try_recv(can_device_t *dev, can_frame_t *frame)
{
	uint32_t base = dev->reg_base;
	uint32_t rxf0s = reg_read(base + FDCAN_RXF0S_OFF);
	uint32_t gidx;
	uint32_t elem;
	uint32_t r0, r1, w0, w1;
	uint32_t dlc;
	uint32_t i;

	if ((rxf0s & FDCAN_RXF0S_F0FL_MASK) == 0u) {
		return CAN_ERR_AGAIN;	/* FIFO empty */
	}

	gidx = (rxf0s >> FDCAN_RXF0S_F0GI_SH) & FDCAN_RXF0S_F0GI_MASK;
	elem = dev->msgram_base + SRAMCAN_RXF0_OFFSET + gidx * CAN_ELEMENT_SIZE;

	r0 = reg_read(elem + 0x00u);
	r1 = reg_read(elem + 0x04u);
	w0 = reg_read(elem + 0x08u);
	w1 = reg_read(elem + 0x0Cu);

	frame->id       = (r0 >> 18) & 0x7FFu;	/* standard ID */
	frame->extended = false;
	frame->remote   = ((r0 & (1u << 29)) != 0u);

	dlc = (r1 >> 16) & 0x0Fu;
	if (dlc > CAN_MAX_DLC) {
		dlc = CAN_MAX_DLC;	/* Classic CAN clamps to 8 */
	}
	frame->len = (uint8_t)dlc;

	for (i = 0u; i < CAN_MAX_DLC; i++) {
		frame->data[i] = (i < 4u)
			? (uint8_t)(w0 >> (8u * i))
			: (uint8_t)(w1 >> (8u * (i - 4u)));
	}

	/* Release the hardware FIFO element (write the get index to RXF0A). */
	reg_write(base + FDCAN_RXF0A_OFF, gidx);

	return CAN_OK;
}

static can_result_t h7_get_status(can_device_t *dev, can_status_t *status)
{
	uint32_t base = dev->reg_base;
	uint32_t psr = reg_read(base + FDCAN_PSR_OFF);
	uint32_t ecr = reg_read(base + FDCAN_ECR_OFF);
	uint32_t ir  = reg_read(base + FDCAN_IR_OFF);

	status->last_error_code = (uint8_t)(psr & FDCAN_PSR_LEC_MASK);
	status->error_passive   = ((psr & FDCAN_PSR_EP) != 0u);
	status->bus_off         = ((psr & FDCAN_PSR_BO) != 0u);
	status->tx_error_count  = (uint8_t)(ecr & 0xFFu);
	status->rx_error_count  = (uint8_t)((ecr >> 8) & 0x7Fu);

	status->rx_overflow = ((ir & FDCAN_IR_RF0L) != 0u);
	if (status->rx_overflow) {
		reg_write(base + FDCAN_IR_OFF, FDCAN_IR_RF0L);	/* clear (w1c) */
	}

	return CAN_OK;
}

/* ------------------------------------------------------------------------ */
/*  Exported ops table - referenced by the board layer.                     */
/* ------------------------------------------------------------------------ */
const can_hw_ops_t can_hw_stm32h7_fdcan_ops = {
	.init       = h7_init,
	.start      = h7_start,
	.stop       = h7_stop,
	.try_send   = h7_try_send,
	.try_recv   = h7_try_recv,
	.get_status = h7_get_status,
};
