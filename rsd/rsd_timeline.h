#ifndef RSD_TIMELINE_H
#define RSD_TIMELINE_H

#include <stdint.h>

/* RTP-Info's random rtptime denotes the SAME capture instant for both
 * tracks (RFC 2326 12.33 / RFC 3550 6.4.1). Rebasing each first sample to
 * zero independently loses their initial A/V offset, which the receiver
 * then abruptly restores when the first pair of RTCP SRs arrives.
 *
 * Negative preroll and RTP wrap are represented modulo 2^32. Split the
 * conversion so a long-running session does not overflow delta * rate.
 * The clock and the capture timestamp must both be CLOCK_MONOTONIC. */
static inline uint32_t rsd_timeline_ticks(int64_t capture_us, int64_t epoch_us,
					uint32_t rate)
{
	int64_t delta = capture_us - epoch_us;
	return (uint32_t)(delta / 1000000) * rate +
		(uint32_t)((delta % 1000000) * rate / 1000000);
}

static inline uint32_t rsd_timeline_offset(uint32_t global_ts, int64_t capture_us,
					int64_t epoch_us, uint32_t rate)
{
	return global_ts - rsd_timeline_ticks(capture_us, epoch_us, rate);
}
#endif
