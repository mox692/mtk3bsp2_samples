/*
 * can.c - CAN Core (logical layer).
 *
 * Receives the public API calls, validates arguments, manages the driver
 * lifecycle state, computes the bit timing, and forwards the request to the
 * selected hardware backend through the can_hw_ops_t table.  This file must
 * not contain any STM32-specific register access or uT-Kernel dependency.
 */
#include <stddef.h>

#include <can/can.h>
#include "internal/can_hw.h"

/* ------------------------------------------------------------------------ */
/*  Bit timing computation                                                  */
/*                                                                          */
/*  Given the FDCAN kernel clock and the desired nominal bit rate, we pick  */
/*  a prescaler so that one bit is divided into a reasonable number of time  */
/*  quanta (tq), then split those quanta to land the sample point near      */
/*  87.5% (the CiA-recommended value for Classic CAN at 500 kbps).          */
/*                                                                          */
/*    tq_per_bit = kernel_clock / (bitrate * brp)                           */
/*    1 (sync) + tseg1 + tseg2 = tq_per_bit                                 */
/*    sample point = (1 + tseg1) / tq_per_bit  ~= 0.875                     */
/*                                                                          */
/*  For 500 kbps @ 80 MHz this yields brp=10, tq=16, tseg1=13, tseg2=2,     */
/*  matching the hand-tuned value used by the original H533 bring-up code.  */
/* ------------------------------------------------------------------------ */

/*
 * Acceptable time-quanta-per-bit window, plus the nominal value we aim for.
 * Among the prescalers that divide the clock exactly, we pick the one whose
 * tq count is closest to CAN_TQ_NOMINAL (tie broken toward the larger tq).
 * For 500 kbps @ 80 MHz this selects brp=10, tq=16, matching the hand-tuned
 * value used by the original H533 bring-up code (sample point 87.5%).
 */
#define CAN_TQ_MAX		25u
#define CAN_TQ_MIN		8u
#define CAN_TQ_NOMINAL		16u
#define CAN_SAMPLE_POINT_PCT	875u	/* 87.5% expressed in per-mille/10 */

static uint32_t abs_diff(uint32_t a, uint32_t b)
{
	return (a > b) ? (a - b) : (b - a);
}

static can_result_t compute_bit_timing(uint32_t kernel_clock_hz,
				       uint32_t bitrate,
				       can_bit_timing_t *out)
{
	uint32_t brp;
	uint32_t best_brp = 0;
	uint32_t best_tq  = 0;

	if (kernel_clock_hz == 0u || bitrate == 0u) {
		return CAN_ERR_INVALID_ARG;
	}

	/*
	 * Search for a prescaler that divides the clock into an integral
	 * number of time quanta per bit.  brp range 1..512 matches FDCAN NBTP.
	 */
	for (brp = 1u; brp <= 512u; brp++) {
		uint32_t denom = bitrate * brp;
		uint32_t tq;

		if ((kernel_clock_hz % denom) != 0u) {
			continue;	/* need an exact division */
		}
		tq = kernel_clock_hz / denom;

		if (tq < CAN_TQ_MIN || tq > CAN_TQ_MAX) {
			continue;
		}
		/* Closest to nominal wins; on a tie prefer the larger tq. */
		if (best_tq == 0u ||
		    abs_diff(tq, CAN_TQ_NOMINAL) < abs_diff(best_tq, CAN_TQ_NOMINAL) ||
		    (abs_diff(tq, CAN_TQ_NOMINAL) == abs_diff(best_tq, CAN_TQ_NOMINAL) &&
		     tq > best_tq)) {
			best_tq  = tq;
			best_brp = brp;
		}
	}

	if (best_tq == 0u) {
		return CAN_ERR_INVALID_ARG;	/* no usable timing found */
	}

	/*
	 * Place the sample point: tseg1 covers everything before the sample
	 * point except the 1 tq sync segment.
	 *   1 + tseg1 = round(best_tq * sample_point)
	 */
	{
		uint32_t before = (best_tq * CAN_SAMPLE_POINT_PCT + 500u) / 1000u;
		uint32_t tseg1;
		uint32_t tseg2;

		if (before < 2u) {
			before = 2u;		/* need at least sync + 1 */
		}
		if (before > best_tq - 1u) {
			before = best_tq - 1u;	/* leave >=1 tq for tseg2 */
		}
		tseg1 = before - 1u;		/* subtract the sync segment */
		tseg2 = best_tq - 1u - tseg1;

		out->brp   = best_brp;
		out->tseg1 = tseg1;
		out->tseg2 = tseg2;
		/* SJW must not exceed tseg2; a common choice is min(4, tseg2). */
		out->sjw   = (tseg2 < 4u) ? tseg2 : 4u;
	}

	return CAN_OK;
}

/* ------------------------------------------------------------------------ */
/*  Argument validation                                                     */
/* ------------------------------------------------------------------------ */
static can_result_t validate_frame(const can_frame_t *frame)
{
	if (frame == NULL) {
		return CAN_ERR_INVALID_ARG;
	}
	/* Phase 1: standard 11-bit IDs only. */
	if (frame->extended) {
		return CAN_ERR_INVALID_ARG;
	}
	if (frame->id > 0x7FFu) {
		return CAN_ERR_INVALID_ARG;
	}
	if (frame->len > CAN_MAX_DLC) {
		return CAN_ERR_INVALID_ARG;
	}
	return CAN_OK;
}

/* ------------------------------------------------------------------------ */
/*  Public API                                                              */
/* ------------------------------------------------------------------------ */
can_result_t can_init(can_device_t *dev, const can_config_t *config)
{
	can_bit_timing_t timing;
	can_result_t res;

	if (dev == NULL || config == NULL || dev->ops == NULL ||
	    dev->ops->init == NULL) {
		return CAN_ERR_INVALID_ARG;
	}

	res = compute_bit_timing(dev->kernel_clock_hz,
				 config->nominal_bitrate, &timing);
	if (res != CAN_OK) {
		return res;
	}

	res = dev->ops->init(dev, &timing, config);
	if (res != CAN_OK) {
		return res;
	}

	dev->state = CAN_STATE_STOPPED;
	return CAN_OK;
}

can_result_t can_start(can_device_t *dev)
{
	can_result_t res;

	if (dev == NULL || dev->ops == NULL || dev->ops->start == NULL) {
		return CAN_ERR_INVALID_ARG;
	}
	if (dev->state == CAN_STATE_UNINIT) {
		return CAN_ERR_NOT_READY;
	}

	res = dev->ops->start(dev);
	if (res == CAN_OK) {
		dev->state = CAN_STATE_RUNNING;
	}
	return res;
}

can_result_t can_stop(can_device_t *dev)
{
	can_result_t res;

	if (dev == NULL || dev->ops == NULL || dev->ops->stop == NULL) {
		return CAN_ERR_INVALID_ARG;
	}
	if (dev->state != CAN_STATE_RUNNING) {
		return CAN_ERR_NOT_READY;
	}

	res = dev->ops->stop(dev);
	if (res == CAN_OK) {
		dev->state = CAN_STATE_STOPPED;
	}
	return res;
}

can_result_t can_try_send(can_device_t *dev, const can_frame_t *frame)
{
	can_result_t res;

	if (dev == NULL || dev->ops == NULL || dev->ops->try_send == NULL) {
		return CAN_ERR_INVALID_ARG;
	}
	res = validate_frame(frame);
	if (res != CAN_OK) {
		return res;
	}
	if (dev->state != CAN_STATE_RUNNING) {
		return CAN_ERR_NOT_READY;
	}

	return dev->ops->try_send(dev, frame);
}

can_result_t can_try_recv(can_device_t *dev, can_frame_t *frame)
{
	if (dev == NULL || frame == NULL || dev->ops == NULL ||
	    dev->ops->try_recv == NULL) {
		return CAN_ERR_INVALID_ARG;
	}
	if (dev->state != CAN_STATE_RUNNING) {
		return CAN_ERR_NOT_READY;
	}

	return dev->ops->try_recv(dev, frame);
}

can_result_t can_get_status(can_device_t *dev, can_status_t *status)
{
	if (dev == NULL || status == NULL || dev->ops == NULL ||
	    dev->ops->get_status == NULL) {
		return CAN_ERR_INVALID_ARG;
	}
	if (dev->state == CAN_STATE_UNINIT) {
		return CAN_ERR_NOT_READY;
	}

	return dev->ops->get_status(dev, status);
}
