#include <tk/tkernel.h>
#include <tm/tmonitor.h>

/* ======================================================================== */
/*  Minimal CAN bring-up for NUCLEO-H533RE (FDCAN1, Classic CAN, 500 kbps)  */
/*                                                                          */
/*  - Transceiver : MCP2562FD  (PB7->TXD, PB8<-RXD, both AF9)               */
/*  - Bus         : SH-C30A (PC) <-> STM32,  500 kbps Classic CAN           */
/*  - Pins        : PB7 = FDCAN1_TX (CN10 pin 5)                            */
/*                  PB8 = FDCAN1_RX (CN10 pin 36)                           */
/*  - RX          : accept every frame into RX FIFO0                        */
/*  - TX          : Standard ID 0x123, DLC=8                                */
/*                                                                          */
/*  No HAL is used; direct register access (in_w / out_w) like the H723     */
/*  sample.  NOTE: STM32H5's FDCAN differs a lot from the STM32H7:          */
/*    * the message RAM uses a FIXED layout (STM32G4-style), so there are   */
/*      no RXESC/TXESC/RXF0C/SIDFC/CCU registers to configure;              */
/*    * RX/TX filtering is done through the single RXGFC register.          */
/*  The FDCAN kernel clock (PLL1Q = 80 MHz) is set up in Core/Src/main.c    */
/*  (SystemClock_Config), because FDCAN cannot be clocked from HSI.         */
/* ======================================================================== */

/* --- Peripheral base addresses (STM32H533, TrustZone disabled => Secure) --- */
#define RCC_BASE_ADDR		0x54020C00u
#define RCC_AHB2ENR		(RCC_BASE_ADDR + 0x008Cu)	/* GPIOx clock enable */
#define RCC_APB1HENR		(RCC_BASE_ADDR + 0x00A0u)	/* FDCAN clock enable */

#define RCC_AHB2ENR_GPIOBEN	(1u << 1)
#define RCC_APB1HENR_FDCANEN	(1u << 9)

/* PB7 = FDCAN1_TX, PB8 = FDCAN1_RX, both Alternate Function 9. */
/* GPIO_MODER(B)/GPIO_AFRL(B)/GPIO_AFRH(B) come from the BSP sysdef.h.   */
#define FDCAN1_AF		9u

/* --- FDCAN1 registers (offsets from FDCAN1 base, RM0481) --- */
#define FDCAN1_BASE_ADDR	0x5000A400u
#define SRAMCAN_BASE_ADDR	0x5000AC00u	/* shared message RAM (fixed layout) */

#define FDCAN_CCCR		(FDCAN1_BASE_ADDR + 0x018u)
#define FDCAN_NBTP		(FDCAN1_BASE_ADDR + 0x01Cu)
#define FDCAN_ECR		(FDCAN1_BASE_ADDR + 0x040u)	/* Error counters   */
#define FDCAN_PSR		(FDCAN1_BASE_ADDR + 0x044u)	/* Protocol status  */
#define FDCAN_IR		(FDCAN1_BASE_ADDR + 0x050u)	/* Interrupt reg    */
#define FDCAN_RXGFC		(FDCAN1_BASE_ADDR + 0x080u)	/* Global filter cfg*/
#define FDCAN_RXF0S		(FDCAN1_BASE_ADDR + 0x090u)	/* RX FIFO0 status  */
#define FDCAN_RXF0A		(FDCAN1_BASE_ADDR + 0x094u)	/* RX FIFO0 ack     */
#define FDCAN_TXBC		(FDCAN1_BASE_ADDR + 0x0C0u)	/* TX buffer config */
#define FDCAN_TXFQS		(FDCAN1_BASE_ADDR + 0x0C4u)	/* TX FIFO/Q status */
#define FDCAN_TXBAR		(FDCAN1_BASE_ADDR + 0x0CCu)	/* TX buffer add req*/

#define FDCAN_IR_RF0L		(1u << 3)	/* RX FIFO0 message lost (overflow) */

#define FDCAN_CCCR_INIT		(1u << 0)
#define FDCAN_CCCR_CCE		(1u << 1)
#define FDCAN_CCCR_FDOE		(1u << 8)	/* FD operation enable   */
#define FDCAN_CCCR_BRSE		(1u << 9)	/* bit rate switching    */

/* --- Fixed message RAM layout (byte offsets from SRAMCAN base, RM0481) --- */
/*  Each RX/TX element is 18 words (72 bytes): 2 header words + 64B data.    */
/*  For Classic CAN we only use the first 8 data bytes (2 data words).       */
#define CAN_ELEMENT_SIZE	72u		/* bytes per RX/TX element */
#define SRAMCAN_RXF0_OFFSET	0x0B0u		/* RX FIFO0 : 3 elements   */
#define SRAMCAN_TXB_OFFSET	0x278u		/* TX buffers: 3 elements  */

#define CAN_TEST_ID		0x123u		/* Standard ID used for TX */

/*
 * Bit timing for Classic CAN 500 kbps @ FDCAN kernel clock = PLL1Q = 80 MHz.
 *   prescaler = 10  ->  time-quantum clock = 8 MHz,  16 tq/bit (8MHz / 500k)
 *   Sync(1) + TSEG1(13) + TSEG2(2) = 16  -> sample point 87.5%
 *   NBTP = (NSJW-1)<<25 | (NBRP-1)<<16 | (NTSEG1-1)<<8 | (NTSEG2-1)<<0
 *        = (2-1)<<25 | (10-1)<<16 | (13-1)<<8 | (2-1)<<0
 */
#define FDCAN_NBTP_500K_80MHZ	((1u << 25) | (9u << 16) | (12u << 8) | (1u << 0))

/* ------------------------------------------------------------------------ */
/*  FDCAN1 initialization                                                   */
/* ------------------------------------------------------------------------ */
LOCAL void can1_init(void)
{
	/* 1. Clocks: GPIOB + FDCAN.  (FDCAN kernel clock source = PLL1Q is    */
	/*    selected in Core/Src/main.c SystemClock_Config.)                 */
	out_w(RCC_AHB2ENR,  in_w(RCC_AHB2ENR)  | RCC_AHB2ENR_GPIOBEN);
	out_w(RCC_APB1HENR, in_w(RCC_APB1HENR) | RCC_APB1HENR_FDCANEN);

	/* 2. PB7 / PB8 -> Alternate Function 9 (FDCAN1_TX / FDCAN1_RX).       */
	{
		UW moder = in_w(GPIO_MODER(B));
		moder &= ~((3u << (7 * 2)) | (3u << (8 * 2)));
		moder |=  ((2u << (7 * 2)) | (2u << (8 * 2)));	/* 0b10 = AF mode */
		out_w(GPIO_MODER(B), moder);

		/* PB7 is in AFRL (pins 0..7), PB8 is in AFRH (pins 8..15). */
		UW afrl = in_w(GPIO_AFRL(B));
		afrl &= ~(0xFu << (7 * 4));
		afrl |=  (FDCAN1_AF << (7 * 4));
		out_w(GPIO_AFRL(B), afrl);

		UW afrh = in_w(GPIO_AFRH(B));
		afrh &= ~(0xFu << ((8 - 8) * 4));
		afrh |=  (FDCAN1_AF << ((8 - 8) * 4));
		out_w(GPIO_AFRH(B), afrh);
	}

	/* 3. Enter INIT + configuration-change-enable */
	out_w(FDCAN_CCCR, in_w(FDCAN_CCCR) | FDCAN_CCCR_INIT);
	while ((in_w(FDCAN_CCCR) & FDCAN_CCCR_INIT) == 0) /* wait */;
	out_w(FDCAN_CCCR, in_w(FDCAN_CCCR) | FDCAN_CCCR_CCE);

	/* Classic CAN only: make sure FD operation / bit-rate switching are off */
	out_w(FDCAN_CCCR, in_w(FDCAN_CCCR) & ~(FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE));

	/* 4. Bit timing: 500 kbps */
	out_w(FDCAN_NBTP, FDCAN_NBTP_500K_80MHZ);

	/* 5. Accept-all into RX FIFO0.                                        */
	/*    RXGFC: ANFS[5:4]=00 (non-matching std -> FIFO0),                 */
	/*           ANFE[3:2]=00 (non-matching ext -> FIFO0),                 */
	/*           LSS/LSE = 0 (no acceptance filters),                      */
	/*           RRFS/RRFE = 0 (accept remote frames too).                 */
	out_w(FDCAN_RXGFC, 0u);

	/* 6. TX: use TX FIFO mode (TFQM=0). In the fixed RAM layout the size  */
	/*    and start address are hard-wired, so nothing else to configure.  */
	out_w(FDCAN_TXBC, in_w(FDCAN_TXBC) & ~(1u << 24));	/* TFQM=0 -> FIFO */

	/* 7. Leave INIT -> Normal mode */
	out_w(FDCAN_CCCR, in_w(FDCAN_CCCR) & ~(FDCAN_CCCR_CCE | FDCAN_CCCR_INIT));
	while ((in_w(FDCAN_CCCR) & FDCAN_CCCR_INIT) != 0) /* wait for normal mode */;
}

/* ------------------------------------------------------------------------ */
/*  Send one Classic CAN frame (Standard ID, up to 8 bytes)                 */
/*                                                                          */
/*  Returns 1 if the frame was queued, 0 if the TX FIFO was full.           */
/* ------------------------------------------------------------------------ */
LOCAL INT can1_send(UW id, const UB *data, UW dlc)
{
	UW txfqs = in_w(FDCAN_TXFQS);
	UW pidx;
	UW elem;
	UW t1;

	/* Bail out if the TX FIFO is full (TFQF = bit21). */
	if ((txfqs & (1u << 21)) != 0) {
		return 0;
	}

	pidx = (txfqs >> 16) & 0x3u;			/* TFQPI: put index (0..2) */
	elem = SRAMCAN_BASE_ADDR + SRAMCAN_TXB_OFFSET + pidx * CAN_ELEMENT_SIZE;

	if (dlc > 8) dlc = 8;

	/* T0: standard ID is left-aligned in bits[28:18], XTD=0, RTR=0 */
	out_w(elem + 0x00u, (id & 0x7FFu) << 18);
	/* T1: DLC in bits[19:16], classic CAN (FDF=0, BRS=0) */
	t1 = (dlc << 16);
	out_w(elem + 0x04u, t1);

	/* Data bytes -> 2 data words (little endian) */
	{
		UW w0 = 0, w1 = 0;
		UW i;
		for (i = 0; i < dlc; i++) {
			if (i < 4) w0 |= ((UW)data[i]) << (8 * i);
			else       w1 |= ((UW)data[i]) << (8 * (i - 4));
		}
		out_w(elem + 0x08u, w0);
		out_w(elem + 0x0Cu, w1);
	}

	/* Request transmission of this buffer */
	out_w(FDCAN_TXBAR, (1u << pidx));

	return 1;
}

/* ------------------------------------------------------------------------ */
/*  Report (and clear) an RX FIFO0 message-loss/overflow event.             */
/* ------------------------------------------------------------------------ */
LOCAL void can1_check_rx_overflow(void)
{
	if ((in_w(FDCAN_IR) & FDCAN_IR_RF0L) != 0) {
		tm_printf((UB*)"CAN RX FIFO OVERFLOW!\n");
		out_w(FDCAN_IR, FDCAN_IR_RF0L);	/* clear by writing 1 */
	}
}

/* ------------------------------------------------------------------------ */
/*  Drain RX FIFO0, printing every received frame. Returns #frames read.    */
/* ------------------------------------------------------------------------ */
LOCAL INT can1_poll_recv(void)
{
	INT n = 0;

	while ((in_w(FDCAN_RXF0S) & 0xFu) != 0) {		/* F0FL: fill level */
		UW gidx = (in_w(FDCAN_RXF0S) >> 8) & 0x3u;	/* F0GI: get index  */
		UW elem = SRAMCAN_BASE_ADDR + SRAMCAN_RXF0_OFFSET + gidx * CAN_ELEMENT_SIZE;

		UW r0  = in_w(elem + 0x00u);
		UW r1  = in_w(elem + 0x04u);
		UW w0  = in_w(elem + 0x08u);
		UW w1  = in_w(elem + 0x0Cu);

		UW id  = (r0 >> 18) & 0x7FFu;			/* standard ID */
		UW dlc = (r1 >> 16) & 0x0Fu;
		UB data[8];
		UW i;

		for (i = 0; i < 8; i++) {
			data[i] = (i < 4) ? (UB)(w0 >> (8 * i))
					  : (UB)(w1 >> (8 * (i - 4)));
		}

		/* Release the hardware FIFO element before the slow tm_printf. */
		out_w(FDCAN_RXF0A, gidx);
		n++;

		tm_printf((UB*)"CAN RX: ID=0x%x DLC=%d data=", id, dlc);
		for (i = 0; i < dlc; i++) {
			tm_printf((UB*)"%x ", data[i]);
		}
		tm_printf((UB*)"\n");
	}
	return n;
}

/* ------------------------------------------------------------------------ */
/*  Print CAN protocol status: last-error-code, bus state, error counters   */
/*                                                                          */
/*  LEC (PSR[2:0]):  0=NoError 1=Stuff 2=Form 3=AckError 4=Bit1             */
/*                   5=Bit0 6=CRC 7=NoChange                                 */
/*  ACT (PSR[4:3]):  0=Sync 1=Idle 2=Receiver 3=Transmitter                 */
/*  EP  (PSR[5]) : error-passive,  BO (PSR[7]) : bus-off                    */
/*  ECR: TEC = bits[7:0], REC = bits[14:8]                                   */
/* ------------------------------------------------------------------------ */
LOCAL void can1_print_status(void)
{
	UW psr = in_w(FDCAN_PSR);
	UW ecr = in_w(FDCAN_ECR);

	tm_printf((UB*)"CAN PSR: LEC=%d ACT=%d EP=%d BO=%d | TEC=%d REC=%d\n",
		  (psr & 0x7u), ((psr >> 3) & 0x3u),
		  ((psr >> 5) & 1u), ((psr >> 7) & 1u),
		  (ecr & 0xFFu), ((ecr >> 8) & 0x7Fu));
}

LOCAL void task_1(INT stacd, void *exinf);	// task execution function
LOCAL ID	tskid_1;			// Task ID number
LOCAL T_CTSK ctsk_1 = {				// Task creation information
	.itskpri	= 10,
	.stksz		= 1024,
	.task		= task_1,
	.tskatr		= TA_HLNG | TA_RNG3,
};

LOCAL void task_can(INT stacd, void *exinf);	// CAN send/receive task
LOCAL ID	tskid_can;
LOCAL T_CTSK ctsk_can = {
	.itskpri	= 9,
	.stksz		= 1024,
	.task		= task_can,
	.tskatr		= TA_HLNG | TA_RNG3,
};

LOCAL void task_1(INT stacd, void *exinf)
{
	while(1) {
		/* Inverts the LED on the board (PA5). */
		out_w(GPIO_ODR(A), (in_w(GPIO_ODR(A)))^(1<<5));

		tk_dly_tsk(100);
	}
}

/* CAN: poll RX FIFO0 continuously, send a test frame about once per second */
LOCAL void task_can(INT stacd, void *exinf)
{
	UW tick = 0;
	UB tx[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

	while(1) {
		can1_poll_recv();
		can1_check_rx_overflow();	/* warn if RX FIFO0 dropped frames */

		if ((tick % 200) == 0) {	/* 200 * 5ms = ~1s */
			tx[0]++;		/* vary payload so frames are distinguishable */
			if (can1_send(CAN_TEST_ID, tx, 8)) {
				tm_printf((UB*)"CAN TX: ID=0x%x queued\n", CAN_TEST_ID);
			} else {
				tm_printf((UB*)"CAN TX: ID=0x%x FIFO FULL\n", CAN_TEST_ID);
			}
			can1_print_status();	/* dump LEC / bus-state / TEC-REC */
		}

		tick++;
		tk_dly_tsk(5);			/* poll RX ~every 5ms */
	}
}

/* usermain function */
EXPORT INT usermain(void)
{
	tm_putstring((UB*)"Start User-main program.\n");

	/* Turn off the LED on the board (PA5). */
	out_w(GPIO_ODR(A), (in_w(GPIO_ODR(A)))&~(1<<5));

	/* Bring up FDCAN1 (Classic CAN 500kbps, PB7=TX / PB8=RX) */
	can1_init();

	/* Create & Start Tasks */
	tskid_1 = tk_cre_tsk(&ctsk_1);
	tk_sta_tsk(tskid_1, 0);

	tskid_can = tk_cre_tsk(&ctsk_can);
	tk_sta_tsk(tskid_can, 0);

	tk_slp_tsk(TMO_FEVR);

	return 0;
}
