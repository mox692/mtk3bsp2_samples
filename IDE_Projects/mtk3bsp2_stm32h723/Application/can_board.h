/*
 * can_board.h - Board support for CAN on the STM32H723 board.
 *
 * The board layer owns everything that is specific to *this* board rather
 * than to the FDCAN controller itself: which GPIO pins carry TX/RX, their
 * alternate function, the peripheral clock enables, the FDCAN kernel clock
 * source, and the FDCAN kernel clock frequency.  It hands the CAN Core a
 * ready-to-use device handle.
 *
 * A different STM32H723 board (different pins / clock tree) only needs its
 * own can_board.c; the driver in Components/can/ stays untouched.  This
 * header is intentionally identical in shape to the H533 board header so the
 * application code (app_main.c) is portable across both boards.
 */
#ifndef CAN_BOARD_H
#define CAN_BOARD_H

#include <can/can.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Enable the FDCAN + GPIO clocks, select the FDCAN kernel clock source and
 * route the TX/RX pins.  Must be called once before can_init().  Returns 0 on
 * success, non-zero on failure.
 */
int can_board_init(void);

/* Return the CAN device handle for FDCAN1 on this board. */
can_device_t *can_board_get_device(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_BOARD_H */
