/*
 * can_board.c - NUCLEO-H533RE board support for the CAN driver.
 *
 * Responsibilities (board-specific, NOT controller-specific):
 *   - enable the GPIOB and FDCAN peripheral clocks
 *   - route PB7 = FDCAN1_TX, PB8 = FDCAN1_RX to alternate function 9
 *   - describe the FDCAN1 instance (register/message-RAM base, kernel clock)
 *     to the CAN Core via a can_device_t handle
 *
 * Transceiver: MCP2562FD (PB7->TXD / PB8<-RXD).  The FDCAN kernel clock is
 * PLL1Q = 80 MHz, configured in Core/Src/main.c SystemClock_Config().
 */
#include <tk/tkernel.h>

#include <can/can.h>
#include "internal/can_hw.h"	/* board constructs the device handle */

#include "can_board.h"

/* --- STM32H533 addresses (TrustZone disabled => Secure alias) --- */
#define RCC_BASE_ADDR		0x54020C00u
#define RCC_AHB2ENR		(RCC_BASE_ADDR + 0x008Cu)	/* GPIOx clock */
#define RCC_APB1HENR		(RCC_BASE_ADDR + 0x00A0u)	/* FDCAN clock */

#define RCC_AHB2ENR_GPIOBEN	(1u << 1)
#define RCC_APB1HENR_FDCANEN	(1u << 9)

#define FDCAN1_BASE_ADDR	0x5000A400u
#define SRAMCAN_BASE_ADDR	0x5000AC00u	/* fixed-layout message RAM */
#define FDCAN1_KERNEL_CLK_HZ	80000000u	/* PLL1Q, see SystemClock_Config */

#define FDCAN1_AF		9u		/* PB7/PB8 alternate function */
#define CAN_TX_PIN		7u
#define CAN_RX_PIN		8u

/* The single FDCAN1 device handle for this board. */
static can_device_t can1_device = {
	.ops             = &can_hw_stm32h5_fdcan_ops,
	.reg_base        = FDCAN1_BASE_ADDR,
	.msgram_base     = SRAMCAN_BASE_ADDR,
	.kernel_clock_hz = FDCAN1_KERNEL_CLK_HZ,
	.hw_priv         = 0,
	.state           = CAN_STATE_UNINIT,
};

int can_board_init(void)
{
	/* 1. Enable GPIOB + FDCAN peripheral clocks. */
	out_w(RCC_AHB2ENR,  in_w(RCC_AHB2ENR)  | RCC_AHB2ENR_GPIOBEN);
	out_w(RCC_APB1HENR, in_w(RCC_APB1HENR) | RCC_APB1HENR_FDCANEN);

	/* 2. PB7 / PB8 -> Alternate Function 9 (FDCAN1_TX / FDCAN1_RX). */
	{
		UW moder = in_w(GPIO_MODER(B));
		moder &= ~((3u << (CAN_TX_PIN * 2)) | (3u << (CAN_RX_PIN * 2)));
		moder |=  ((2u << (CAN_TX_PIN * 2)) | (2u << (CAN_RX_PIN * 2)));
		out_w(GPIO_MODER(B), moder);

		/* PB7 lives in AFRL (pins 0..7), PB8 in AFRH (pins 8..15). */
		UW afrl = in_w(GPIO_AFRL(B));
		afrl &= ~(0xFu << (CAN_TX_PIN * 4));
		afrl |=  (FDCAN1_AF << (CAN_TX_PIN * 4));
		out_w(GPIO_AFRL(B), afrl);

		UW afrh = in_w(GPIO_AFRH(B));
		afrh &= ~(0xFu << ((CAN_RX_PIN - 8u) * 4));
		afrh |=  (FDCAN1_AF << ((CAN_RX_PIN - 8u) * 4));
		out_w(GPIO_AFRH(B), afrh);
	}

	return 0;
}

can_device_t *can_board_get_device(void)
{
	return &can1_device;
}
