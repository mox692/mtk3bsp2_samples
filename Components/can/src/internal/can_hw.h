/*
 * can_hw.h - Internal interface between the CAN Core (can.c) and the
 *            hardware backends (src/hw/stm32h5_fdcan.c, stm32h7_fdcan.c, ...).
 *
 * This is NOT a public header: applications must not include it.  It defines
 * the operations that every controller backend implements, plus the concrete
 * layout of can_device_t.  The core talks to a backend only through the
 * can_hw_ops_t function-pointer table, so adding a new controller (e.g. the
 * STM32H7 FDCAN) is just a matter of providing another ops table and its
 * backing register access - no changes to can.c are required.
 */
#ifndef CAN_INTERNAL_CAN_HW_H
#define CAN_INTERNAL_CAN_HW_H

#include <can/can.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lifecycle state tracked by the core. */
typedef enum {
	CAN_STATE_UNINIT = 0,
	CAN_STATE_STOPPED,	/* configured, in initialization mode      */
	CAN_STATE_RUNNING	/* normal operation                        */
} can_state_t;

/*
 * Computed bit timing, produced by the core from can_config_t.nominal_bitrate
 * and the backend-reported kernel clock.  Values are the "programmable" form
 * (i.e. the actual quanta counts, not the register-encoded "minus one").
 */
typedef struct {
	uint32_t	brp;	/* baud-rate prescaler   (1..512)            */
	uint32_t	tseg1;	/* PROP+PHASE1 quanta    (2..256)            */
	uint32_t	tseg2;	/* PHASE2 quanta         (2..128)            */
	uint32_t	sjw;	/* (re)sync jump width   (1..128)            */
} can_bit_timing_t;

/*
 * Hardware backend operations.  ctx points at can_device_t so a backend can
 * reach its own register base / kernel clock stored by the board layer.
 */
typedef struct {
	/* Enter init mode, program bit timing + accept-all filter, stay stopped. */
	can_result_t (*init)(can_device_t *dev, const can_bit_timing_t *timing,
			     const can_config_t *config);

	/* Enter / leave normal operating mode. */
	can_result_t (*start)(can_device_t *dev);
	can_result_t (*stop)(can_device_t *dev);

	/* Non-blocking single-frame send/receive. */
	can_result_t (*try_send)(can_device_t *dev, const can_frame_t *frame);
	can_result_t (*try_recv)(can_device_t *dev, can_frame_t *frame);

	/* Snapshot controller status (error counters, bus state, overflow). */
	can_result_t (*get_status)(can_device_t *dev, can_status_t *status);
} can_hw_ops_t;

/*
 * Concrete device handle.
 *
 * The board layer allocates one of these (statically), fills in ops,
 * reg_base and kernel_clock_hz, and hands it to the application via
 * can_board_get_device().  The core owns `state`; the backend may use
 * `hw_priv` for controller-specific bookkeeping if needed.
 */
struct can_device {
	const can_hw_ops_t	*ops;		/* backend operation table   */
	uint32_t		reg_base;	/* FDCAN instance base addr  */
	uint32_t		msgram_base;	/* message RAM base addr     */
	uint32_t		kernel_clock_hz;/* FDCAN kernel clock (Hz)   */
	void			*hw_priv;	/* optional backend state    */

	can_state_t		state;		/* owned by the core         */
};

/* ------------------------------------------------------------------------ */
/*  Backend ops tables (defined by the hw backends, referenced by board).   */
/* ------------------------------------------------------------------------ */
extern const can_hw_ops_t can_hw_stm32h5_fdcan_ops;	/* STM32H5 (H533) */
/* extern const can_hw_ops_t can_hw_stm32h7_fdcan_ops; */ /* added in Phase 2 */

#ifdef __cplusplus
}
#endif

#endif /* CAN_INTERNAL_CAN_HW_H */
