/*
 * can_board.c - STM32H723 board support for the CAN driver.
 *
 * Responsibilities (board-specific, NOT controller-specific):
 *   - enable the GPIOD and FDCAN peripheral clocks
 *   - select the FDCAN kernel clock source (HSE = 8 MHz here)
 *   - route PD1 = FDCAN1_TX, PD0 = FDCAN1_RX to alternate function 9
 *   - describe the FDCAN1 instance (register/message-RAM base, kernel clock)
 *     to the CAN Core via a can_device_t handle
 *
 * Transceiver: MCP2562FD (PD1->TXD / PD0<-RXD).  Unlike the H533 (whose FDCAN
 * kernel clock comes from PLL1Q set up in main.c), the H723 selects HSE as the
 * FDCAN kernel clock directly here, matching the original bring-up code.
 */
#include <tk/tkernel.h>

#include <can/can.h>
#include "internal/can_hw.h"	/* board constructs the device handle */

#include "can_board.h"

/* --- STM32H723 addresses (RM0468 / CMSIS) --- */
#define RCC_BASE_ADDR		0x58024400u
#define RCC_AHB4ENR		(RCC_BASE_ADDR + 0x00E0u)	/* GPIOx clock enable   */
#define RCC_APB1HENR		(RCC_BASE_ADDR + 0x00ECu)	/* FDCAN clock enable   */
#define RCC_D2CCIP1R		(RCC_BASE_ADDR + 0x0050u)	/* FDCAN kernel clk sel */

#define RCC_AHB4ENR_GPIODEN	(1u << 3)
#define RCC_APB1HENR_FDCANEN	(1u << 8)
#define RCC_D2CCIP1R_FDCANSEL	(3u << 28)	/* 00=hse_ck 01=pll1_q 10=pll2_q */

#define FDCAN1_BASE_ADDR	0x4000A000u
#define SRAMCAN_BASE_ADDR	0x4000AC00u	/* shared 10KB message RAM */
#define FDCAN1_KERNEL_CLK_HZ	8000000u	/* HSE, selected below */

#define FDCAN1_AF		9u		/* PD0/PD1 alternate function */
#define CAN_RX_PIN		0u		/* PD0 = FDCAN1_RX */
#define CAN_TX_PIN		1u		/* PD1 = FDCAN1_TX */

/* The single FDCAN1 device handle for this board. */
static can_device_t can1_device = {
	.ops             = &can_hw_stm32h7_fdcan_ops,
	.reg_base        = FDCAN1_BASE_ADDR,
	.msgram_base     = SRAMCAN_BASE_ADDR,
	.kernel_clock_hz = FDCAN1_KERNEL_CLK_HZ,
	.hw_priv         = 0,
	.state           = CAN_STATE_UNINIT,
};

int can_board_init(void)
{
	/* 1. Clocks: GPIOD + FDCAN, and select HSE (8 MHz) as FDCAN kernel clk. */
	out_w(RCC_AHB4ENR,  in_w(RCC_AHB4ENR)  | RCC_AHB4ENR_GPIODEN);
	out_w(RCC_APB1HENR, in_w(RCC_APB1HENR) | RCC_APB1HENR_FDCANEN);
	out_w(RCC_D2CCIP1R, in_w(RCC_D2CCIP1R) & ~RCC_D2CCIP1R_FDCANSEL); /* 00=HSE */

	/* 2. PD0 / PD1 -> Alternate Function 9 (FDCAN1_RX / FDCAN1_TX).
	 *    Both pins live in AFRL (pins 0..7). */
	{
		UW moder = in_w(GPIO_MODER(D));
		moder &= ~((3u << (CAN_RX_PIN * 2)) | (3u << (CAN_TX_PIN * 2)));
		moder |=  ((2u << (CAN_RX_PIN * 2)) | (2u << (CAN_TX_PIN * 2)));
		out_w(GPIO_MODER(D), moder);

		UW afrl = in_w(GPIO_AFRL(D));
		afrl &= ~((0xFu << (CAN_RX_PIN * 4)) | (0xFu << (CAN_TX_PIN * 4)));
		afrl |=  ((FDCAN1_AF << (CAN_RX_PIN * 4)) |
			  (FDCAN1_AF << (CAN_TX_PIN * 4)));
		out_w(GPIO_AFRL(D), afrl);
	}

	return 0;
}

can_device_t *can_board_get_device(void)
{
	return &can1_device;
}
