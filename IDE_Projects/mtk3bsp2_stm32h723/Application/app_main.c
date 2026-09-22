#include <tk/tkernel.h>
#include <tm/tmonitor.h>

/* Rust function declaration */
extern void rust_hello(void);

/* ======================================================================== */
/*  Minimal CAN bring-up (FDCAN1, Classic CAN, 500 kbps, PD0=RX / PD1=TX)   */
/*                                                                          */
/*  Minimal implementation corresponding to instruction.md's                */
/*  "# Actual bring-up procedure" section.                                  */
/*  No HAL is used; written with direct register access (in_w / out_w)      */
/*  just like the existing code.                                            */
/*                                                                          */
/*  - Transceiver: MCP2562FD  (PD1->TXD, PD0<-RXD)                          */
/*  - Bus         : SH-C30A (PC) <-> STM32,  500 kbps Classic CAN           */
/*  - RX          : Standard ID 0x123 を RX FIFO0 で受信                     */
/*  - TX          : Standard ID 0x123, DLC=8 を送信                          */
/* ======================================================================== */

/* --- Peripheral base addresses (STM32H723, RM0468 / CMSIS) --- */
#define RCC_BASE_ADDR		0x58024400u
#define RCC_AHB4ENR		(RCC_BASE_ADDR + 0x00E0u)	/* GPIOx clock enable   */
#define RCC_APB1HENR		(RCC_BASE_ADDR + 0x00ECu)	/* FDCAN clock enable   */
#define RCC_D2CCIP1R		(RCC_BASE_ADDR + 0x0050u)	/* FDCAN kernel clk sel */

#define RCC_AHB4ENR_GPIODEN	(1u << 3)
#define RCC_APB1HENR_FDCANEN	(1u << 8)
#define RCC_D2CCIP1R_FDCANSEL	(3u << 28)	/* 00=hse_ck 01=pll1_q 10=pll2_q */

/* GPIOD (PD0=FDCAN1_RX, PD1=FDCAN1_TX, both Alternate Function 9) */
/* GPIO_MODER(D)/GPIO_AFRL(D)/... macros come from the BSP sysdef.h */
#define FDCAN1_AF		9u

/* --- FDCAN1 registers (offsets from FDCAN1 base, RM0468 ch.61) --- */
#define FDCAN1_BASE_ADDR	0x4000A000u
#define FDCAN_CCU_BASE_ADDR	0x4000A800u
#define SRAMCAN_BASE_ADDR	0x4000AC00u	/* shared 10KB message RAM */

#define FDCAN_CCCR		(FDCAN1_BASE_ADDR + 0x018u)
#define FDCAN_NBTP		(FDCAN1_BASE_ADDR + 0x01Cu)
#define FDCAN_ECR		(FDCAN1_BASE_ADDR + 0x040u)	/* Error counters      */
#define FDCAN_PSR		(FDCAN1_BASE_ADDR + 0x044u)	/* Protocol status     */
#define FDCAN_GFC		(FDCAN1_BASE_ADDR + 0x080u)
#define FDCAN_SIDFC		(FDCAN1_BASE_ADDR + 0x084u)
#define FDCAN_RXF0C		(FDCAN1_BASE_ADDR + 0x0A0u)
#define FDCAN_RXF0S		(FDCAN1_BASE_ADDR + 0x0A4u)
#define FDCAN_RXF0A		(FDCAN1_BASE_ADDR + 0x0A8u)
#define FDCAN_RXESC		(FDCAN1_BASE_ADDR + 0x0BCu)
#define FDCAN_TXBC		(FDCAN1_BASE_ADDR + 0x0C0u)
#define FDCAN_TXFQS		(FDCAN1_BASE_ADDR + 0x0C4u)
#define FDCAN_TXESC		(FDCAN1_BASE_ADDR + 0x0C8u)
#define FDCAN_TXBAR		(FDCAN1_BASE_ADDR + 0x0D0u)
#define FDCAN_IR		(FDCAN1_BASE_ADDR + 0x050u)	/* Interrupt register  */
#define FDCAN_IR_RF0L		(1u << 3)	/* RX FIFO0 message lost (overflow) */

#define FDCAN_CCU_CCFG		(FDCAN_CCU_BASE_ADDR + 0x004u)
#define FDCANCCU_CCFG_BCC	(1u << 6)	/* Bypass clock calibration */

#define FDCAN_CCCR_INIT		(1u << 0)
#define FDCAN_CCCR_CCE		(1u << 1)

/* --- Message RAM layout (byte offsets from SRAMCAN base) ---            */
/*  registers store the start address as a word-aligned byte offset.      */
/*                                                                        */
/*  Classic CAN, 8-byte payload => 8-byte header + 8-byte data = 16 B/el. */
/*    Std ID filter : 0x000-0x003  (1 element  x  4B  =   4 B)            */
/*    RX FIFO0      : 0x040-0x13F  (16 elements x 16B  = 256 B)           */
/*    TX FIFO       : 0x140-0x1BF  ( 8 elements x 16B  = 128 B)           */
/*  Ends at 0x1C0 (448B), well within the shared 10KB message RAM.        */
/*  NOTE: this RAM is shared across FDCAN instances; if FDCAN2 is enabled */
/*  later, its regions must not overlap these offsets.                    */
#define CAN_RX_FIFO_SIZE	16u	/* # of RX FIFO0 elements */
#define CAN_TX_FIFO_SIZE	8u	/* # of TX FIFO elements  */
#define CAN_ELEMENT_SIZE	16u	/* header(8) + data(8) bytes/element */

#define SRAMCAN_FLS_OFFSET	0x000u	/* 11-bit filter list : 1 element (4B) */
#define SRAMCAN_RXF0_OFFSET	0x040u	/* RX FIFO0                            */
#define SRAMCAN_TXB_OFFSET	(SRAMCAN_RXF0_OFFSET + \
				 CAN_RX_FIFO_SIZE * CAN_ELEMENT_SIZE) /* TX FIFO */

#define CAN_TEST_ID		0x123u	/* Standard ID used for both TX and RX */

/*
 * Bit timing for Classic CAN 500 kbps @ FDCAN kernel clock = HSE = 8 MHz.
 *   16 time quanta/bit (8MHz / 500k),  prescaler = 1
 *   Sync(1) + TSEG1(13) + TSEG2(2) = 16   -> sample point 87.5%
 *   NBTP = NSJW-1<<25 | NBRP-1<<16 | NTSEG1-1<<8 | NTSEG2-1<<0
 *        = (2-1)<<25 | (1-1)<<16 | (13-1)<<8 | (2-1)<<0
 */
#define FDCAN_NBTP_500K_8MHZ	((1u << 25) | (0u << 16) | (12u << 8) | (1u << 0))

/* ------------------------------------------------------------------------ */
/*  FDCAN1 initialization                                                   */
/* ------------------------------------------------------------------------ */
LOCAL void can1_init(void)
{
	/* 1. Clocks: GPIOD + FDCAN, and select HSE (8MHz) as FDCAN kernel clk */
	out_w(RCC_AHB4ENR,  in_w(RCC_AHB4ENR)  | RCC_AHB4ENR_GPIODEN);
	out_w(RCC_APB1HENR, in_w(RCC_APB1HENR) | RCC_APB1HENR_FDCANEN);
	out_w(RCC_D2CCIP1R, in_w(RCC_D2CCIP1R) & ~RCC_D2CCIP1R_FDCANSEL); /* 00=HSE */

	/* 2. PD0 / PD1 -> Alternate Function 9 (FDCAN1_RX / FDCAN1_TX) */
	{
		UW moder = in_w(GPIO_MODER(D));
		moder &= ~((3u << (0 * 2)) | (3u << (1 * 2)));
		moder |=  ((2u << (0 * 2)) | (2u << (1 * 2)));	/* 0b10 = AF mode */
		out_w(GPIO_MODER(D), moder);

		UW afrl = in_w(GPIO_AFRL(D));
		afrl &= ~((0xFu << (0 * 4)) | (0xFu << (1 * 4)));
		afrl |=  ((FDCAN1_AF << (0 * 4)) | (FDCAN1_AF << (1 * 4)));
		out_w(GPIO_AFRL(D), afrl);
	}

	/* 3. Enter INIT + configuration-change-enable */
	out_w(FDCAN_CCCR, in_w(FDCAN_CCCR) | FDCAN_CCCR_INIT);
	while ((in_w(FDCAN_CCCR) & FDCAN_CCCR_INIT) == 0) /* wait */;
	out_w(FDCAN_CCCR, in_w(FDCAN_CCCR) | FDCAN_CCCR_CCE);

	/* Classic CAN only: make sure FDOE/BRSE are off (clear all option bits) */
	out_w(FDCAN_CCCR, (in_w(FDCAN_CCCR) & ~0x0000FF00u) | FDCAN_CCCR_INIT | FDCAN_CCCR_CCE);

	/* Bypass the clock calibration unit (use kernel clock directly) */
	out_w(FDCAN_CCU_CCFG, in_w(FDCAN_CCU_CCFG) | FDCANCCU_CCFG_BCC);

	/* 4. Bit timing: 500 kbps */
	out_w(FDCAN_NBTP, FDCAN_NBTP_500K_8MHZ);

	/* 5. Accept-all: route every non-matching frame into RX FIFO0.        */
	/*    GFC.ANFS (bits[5:4]) / ANFE (bits[3:2]) = 00 -> accept into FIFO0.*/
	/*    RRFS (bit0) / RRFE (bit1) = 0 -> also accept remote frames.      */
	/*    (Was (3<<4)|(3<<2) = reject-all, which only let ID 0x123 in.)    */
	out_w(FDCAN_GFC, 0u);

	/* 6. No standard ID filter list needed while accepting all frames.    */
	out_w(FDCAN_SIDFC, 0u);

	/* 7. RX FIFO0: CAN_RX_FIFO_SIZE elements, 8-byte data section.        */
	/*    RXF0C: F0S (FIFO0 size) is bits[22:16], F0SA is the start addr.  */
	out_w(FDCAN_RXF0C, (CAN_RX_FIFO_SIZE << 16) | SRAMCAN_RXF0_OFFSET);
	out_w(FDCAN_RXESC, 0u);	/* F0DS=0 -> 8-byte data field */

	/* 8. TX FIFO: CAN_TX_FIFO_SIZE elements, 8-byte data section.         */
	/*    TXBC: NDTB (dedicated buffers) is bits[21:16], TFQS (FIFO/queue  */
	/*    size) is bits[29:24], TFQM (bit30) selects FIFO(0)/queue(1) mode.*/
	/*    We want a TX FIFO, so the size MUST go in TFQS (<<24), not NDTB. */
	/*    (Previously (1u<<16) wrongly set 1 dedicated buffer + 0 FIFO.)   */
	out_w(FDCAN_TXBC, (CAN_TX_FIFO_SIZE << 24) | SRAMCAN_TXB_OFFSET);
	out_w(FDCAN_TXESC, 0u);	/* TBDS=0 -> 8-byte data field */

	/* 9. Leave INIT -> Normal mode */
	out_w(FDCAN_CCCR, in_w(FDCAN_CCCR) & ~(FDCAN_CCCR_CCE | FDCAN_CCCR_INIT));
	while ((in_w(FDCAN_CCCR) & FDCAN_CCCR_INIT) != 0) /* wait for normal mode */;
}

/* ------------------------------------------------------------------------ */
/*  Send one Classic CAN frame (Standard ID, up to 8 bytes)                 */
/*                                                                          */
/*  Returns 1 if the frame was queued, 0 if the TX FIFO was full.           */
/*  A return of 1 means "queued", not "already transmitted on the bus".     */
/*  Assumes a single task drives the TX FIFO; concurrent callers would      */
/*  need external synchronization.                                          */
/* ------------------------------------------------------------------------ */
LOCAL INT can1_send(UW id, const UB *data, UW dlc)
{
	UW txfqs = in_w(FDCAN_TXFQS);
	UW pidx;
	UW elem;
	UW t1;

	/* Bail out if the TX FIFO is full: TFQF (bit21) set, or TFFL (free    */
	/* level, bits[5:0]) == 0. Writing anyway would clobber a pending frame*/
	/* and is a likely cause of "TX sometimes stops" symptoms.             */
	if ((txfqs & (1u << 21)) != 0) {
		return 0;
	}
	if ((txfqs & 0x3Fu) == 0) {
		return 0;
	}

	pidx = (txfqs >> 16) & 0x1Fu;			/* TFQPI: put index */
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
/*  RF0L only signals that >=1 frame was dropped, not how many.             */
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

	while ((in_w(FDCAN_RXF0S) & 0x7Fu) != 0) {		/* F0FL: fill level */
		UW gidx = (in_w(FDCAN_RXF0S) >> 8) & 0x3Fu;	/* F0GI: get index  */
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

		/* Release the hardware FIFO element BEFORE the slow tm_printf so   */
		/* the slot is available again as quickly as possible. The frame    */
		/* has already been copied into local storage above.                */
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

LOCAL void task_2(INT stacd, void *exinf);	// task execution function
LOCAL ID	tskid_2;			// Task ID number
LOCAL T_CTSK ctsk_2 = {				// Task creation information
	.itskpri	= 10,
	.stksz		= 1024,
	.task		= task_2,
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
		// tm_printf((UB*)"task 1\n");

		/* Inverts the LED on the board. */
		out_w(GPIO_ODR(B), (in_w(GPIO_ODR(B)))^((1<<0)|(1<<14)));
		out_w(GPIO_ODR(E), (in_w(GPIO_ODR(E)))^(1<<1));

		tk_dly_tsk(500);
	}
}

LOCAL void task_2(INT stacd, void *exinf)
{
	while(1) {
		// tm_printf((UB*)"task 2\n");

		/* Call Rust function */
		rust_hello();

		tk_dly_tsk(700);
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
		tk_dly_tsk(5);			/* poll RX ~every 5ms (was 50ms) */
	}
}

/* usermain function */
EXPORT INT usermain(void)
{
	tm_putstring((UB*)"Start User-main program.\n");

	/* Turn off the LED on the board. */
	out_w(GPIO_ODR(B), (in_w(GPIO_ODR(B)))&~((1<<0)|(1<<14)));
	out_w(GPIO_ODR(E), (in_w(GPIO_ODR(E)))&~(1<<1));

	/* Bring up FDCAN1 (Classic CAN 500kbps, PD0/PD1) */
	can1_init();

	/* Create & Start Tasks */
	tskid_1 = tk_cre_tsk(&ctsk_1);
	tk_sta_tsk(tskid_1, 0);

	tskid_2 = tk_cre_tsk(&ctsk_2);
	tk_sta_tsk(tskid_2, 0);

	tskid_can = tk_cre_tsk(&ctsk_can);
	tk_sta_tsk(tskid_can, 0);

	tk_slp_tsk(TMO_FEVR);

	return 0;
}
