/*
 * can.h - Public API for the portable CAN driver.
 *
 * This header is intentionally free of any STM32 (stm32h5xx.h / stm32h7xx.h)
 * or uT-Kernel (tk/tkernel.h) dependency, so that the same API can be used
 * from H533, H723, other MCUs, or even from Rust through the C ABI.
 *
 * Phase 1 scope: Classic CAN, standard 11-bit ID, up to 8 data bytes,
 * polling-based non-blocking send/receive.  CAN FD, extended IDs, interrupts
 * and multi-instance support are added in later phases.
 */
#ifndef CAN_CAN_H
#define CAN_CAN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/*  Result / error codes                                                    */
/* ------------------------------------------------------------------------ */
typedef enum {
	CAN_OK			= 0,	/* success (TX: request accepted by HW)  */
	CAN_ERR_AGAIN		= -1,	/* RX: no frame available                */
	CAN_ERR_BUSY		= -2,	/* TX: FIFO full                         */
	CAN_ERR_INVALID_ARG	= -3,	/* bad ID / DLC / NULL pointer / config  */
	CAN_ERR_NOT_READY	= -4,	/* not initialized / not started         */
	CAN_ERR_BUS_OFF		= -5,	/* controller is in bus-off state        */
	CAN_ERR_TIMEOUT		= -6,	/* HW did not reach expected state       */
	CAN_ERR_IO		= -7	/* generic hardware/backend failure      */
} can_result_t;

/* ------------------------------------------------------------------------ */
/*  CAN frame (Classic CAN, up to 8 data bytes)                             */
/* ------------------------------------------------------------------------ */
#define CAN_MAX_DLC		8u

typedef struct {
	uint32_t	id;		/* 11-bit standard ID (Phase 1)          */
	uint8_t		len;		/* number of valid data bytes (0..8)     */
	uint8_t		data[CAN_MAX_DLC];
	bool		extended;	/* reserved for Phase 3; must be false   */
	bool		remote;		/* remote frame (RTR)                    */
} can_frame_t;

/* ------------------------------------------------------------------------ */
/*  Configuration                                                           */
/* ------------------------------------------------------------------------ */
typedef struct {
	/*
	 * Desired nominal (arbitration) bit rate in bit/s, e.g. 500000.
	 * The driver computes the bit timing from this value together with
	 * the FDCAN kernel clock frequency reported by the hardware backend,
	 * so the same source works for any board clock configuration.
	 */
	uint32_t	nominal_bitrate;

	bool		auto_retransmission;	/* false => DAR (one-shot)       */
	bool		internal_loopback;	/* true  => internal test mode   */

	/* Additional options (sample point, data-phase bitrate, ...) later. */
} can_config_t;

/* ------------------------------------------------------------------------ */
/*  Controller status snapshot                                              */
/* ------------------------------------------------------------------------ */
typedef struct {
	bool		error_passive;	/* PSR.EP                                */
	bool		bus_off;	/* PSR.BO                                */
	uint8_t		last_error_code;/* PSR.LEC                               */
	uint8_t		tx_error_count;	/* ECR.TEC                               */
	uint8_t		rx_error_count;	/* ECR.REC                               */
	bool		rx_overflow;	/* an RX FIFO0 message-lost was observed  */
} can_status_t;

/* ------------------------------------------------------------------------ */
/*  Device handle                                                           */
/*                                                                          */
/*  Opaque to the application.  A concrete instance is provided by the      */
/*  board support layer (see can_board_get_device()).                       */
/* ------------------------------------------------------------------------ */
typedef struct can_device can_device_t;

/* ------------------------------------------------------------------------ */
/*  Public API                                                              */
/* ------------------------------------------------------------------------ */

/*
 * Initialize the controller with the given configuration.
 * The controller is left in a configured-but-stopped state; call can_start()
 * to enter normal operation.  Returns CAN_OK on success.
 */
can_result_t can_init(can_device_t *dev, const can_config_t *config);

/* Enter normal operating mode (able to send/receive on the bus). */
can_result_t can_start(can_device_t *dev);

/* Leave normal operating mode (back to initialization mode). */
can_result_t can_stop(can_device_t *dev);

/*
 * Queue one frame for transmission (non-blocking).
 *   CAN_OK              request accepted by the hardware TX FIFO
 *   CAN_ERR_BUSY        TX FIFO is full
 *   CAN_ERR_INVALID_ARG bad frame (ID range, len > 8, extended in Phase 1)
 *   CAN_ERR_NOT_READY   controller not started
 *
 * A CAN_OK result only means the request was accepted; actual on-bus
 * completion (ACK, retransmission, bus-off) is not awaited here.
 */
can_result_t can_try_send(can_device_t *dev, const can_frame_t *frame);

/*
 * Read one frame from the RX FIFO (non-blocking).
 *   CAN_OK         one frame was copied into *frame
 *   CAN_ERR_AGAIN  no frame available
 */
can_result_t can_try_recv(can_device_t *dev, can_frame_t *frame);

/* Fill *status with a snapshot of the controller state. */
can_result_t can_get_status(can_device_t *dev, can_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* CAN_CAN_H */
