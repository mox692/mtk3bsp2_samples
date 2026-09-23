#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include <can/can.h>
#include "can_board.h"

/* Rust function declaration */
extern void rust_hello(void);

/* ======================================================================== */
/*  CAN sample for the STM32H723 board using the portable CAN driver.       */
/*                                                                          */
/*  All FDCAN register access now lives in the driver (Components/can/) and */
/*  the board support file (can_board.c); this file only uses the public    */
/*  CAN API (can_init / can_start / can_try_send / can_try_recv).  The same  */
/*  application code runs on the NUCLEO-H533RE - only can_board.c and the    */
/*  hardware backend differ between the two boards.                          */
/*                                                                          */
/*  Bus: SH-C30A (PC) <-> STM32, 500 kbps Classic CAN, Standard ID 0x123.   */
/* ======================================================================== */

#define CAN_TEST_ID	0x123u		/* Standard ID used for TX */

static can_device_t *can_dev;

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
		/* Inverts the LEDs on the board. */
		out_w(GPIO_ODR(B), (in_w(GPIO_ODR(B)))^((1<<0)|(1<<14)));
		out_w(GPIO_ODR(E), (in_w(GPIO_ODR(E)))^(1<<1));

		tk_dly_tsk(500);
	}
}

LOCAL void task_2(INT stacd, void *exinf)
{
	while(1) {
		/* Call Rust function */
		rust_hello();

		tk_dly_tsk(700);
	}
}

/* Print a CAN protocol status snapshot obtained from the driver. */
LOCAL void print_can_status(void)
{
	can_status_t st;

	if (can_get_status(can_dev, &st) != CAN_OK) {
		return;
	}
	if (st.rx_overflow) {
		tm_printf((UB*)"CAN RX FIFO OVERFLOW!\n");
	}
	tm_printf((UB*)"CAN PSR: LEC=%d EP=%d BO=%d | TEC=%d REC=%d\n",
		  st.last_error_code, st.error_passive, st.bus_off,
		  st.tx_error_count, st.rx_error_count);
}

/* CAN: poll RX FIFO0 continuously, send a test frame about once per second. */
LOCAL void task_can(INT stacd, void *exinf)
{
	UW tick = 0;
	can_frame_t rx;
	can_frame_t tx = {
		.id       = CAN_TEST_ID,
		.len      = 8,
		.data     = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 },
		.extended = false,
		.remote   = false,
	};

	while(1) {
		/* Drain every received frame (one per call). */
		while (can_try_recv(can_dev, &rx) == CAN_OK) {
			UW i;
			tm_printf((UB*)"CAN RX: ID=0x%x DLC=%d data=", rx.id, rx.len);
			for (i = 0; i < rx.len; i++) {
				tm_printf((UB*)"%x ", rx.data[i]);
			}
			tm_printf((UB*)"\n");
		}

		if ((tick % 200) == 0) {	/* 200 * 5ms = ~1s */
			can_result_t res;
			tx.data[0]++;		/* vary payload so frames differ */
			res = can_try_send(can_dev, &tx);
			if (res == CAN_OK) {
				tm_printf((UB*)"CAN TX: ID=0x%x queued\n", CAN_TEST_ID);
			} else {
				tm_printf((UB*)"CAN TX: ID=0x%x send failed (%d)\n",
					  CAN_TEST_ID, res);
			}
			print_can_status();
		}

		tick++;
		tk_dly_tsk(5);			/* poll RX ~every 5ms */
	}
}

/* usermain function */
EXPORT INT usermain(void)
{
	can_config_t config = {
		.nominal_bitrate     = 500000,
		.auto_retransmission = true,
		.internal_loopback   = false,
	};

	tm_putstring((UB*)"Start User-main program.\n");

	/* Turn off the LEDs on the board. */
	out_w(GPIO_ODR(B), (in_w(GPIO_ODR(B)))&~((1<<0)|(1<<14)));
	out_w(GPIO_ODR(E), (in_w(GPIO_ODR(E)))&~(1<<1));

	/* Bring up FDCAN1 (Classic CAN 500 kbps, PD0=RX / PD1=TX). */
	if (can_board_init() != 0) {
		tm_putstring((UB*)"can_board_init failed\n");
		return -1;
	}
	can_dev = can_board_get_device();

	if (can_init(can_dev, &config) != CAN_OK) {
		tm_putstring((UB*)"can_init failed\n");
		return -1;
	}
	if (can_start(can_dev) != CAN_OK) {
		tm_putstring((UB*)"can_start failed\n");
		return -1;
	}

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
