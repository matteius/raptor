#include "greatest.h"
#include "../rsd/rsd_timeline.h"

TEST timeline_keeps_initial_av_offset(void)
{
	const int64_t epoch = 100000000;
	const uint32_t video_global = 0xfffffff0U, audio_global = 1234;
	const uint32_t video_random = 0xabcdef00U, audio_random = 0x1234abcdU;
	/* Audio captured 80 ms before video: independent first-sample rebases
	 * lose more than one AAC frame and jump backward at RTCP alignment. */
	uint32_t vo = rsd_timeline_offset(video_global, epoch + 20000, epoch, 90000);
	uint32_t ao = rsd_timeline_offset(audio_global, epoch - 60000, epoch, 16000);
	uint32_t v = video_global - vo + video_random;
	uint32_t a = audio_global - ao + audio_random;
	ASSERT_EQ(1800, (int32_t)(v - video_random));
	ASSERT_EQ(-960, (int32_t)(a - audio_random));
	/* Converting either RTP-Info-relative timestamp back to capture time
	 * recovers the same origin, including preroll and modular wrap. */
	ASSERT_EQ(epoch, epoch + 20000 - (int32_t)(v - video_random) * 1000000LL / 90000);
	ASSERT_EQ(epoch, epoch - 60000 - (int32_t)(a - audio_random) * 1000000LL / 16000);
	ASSERT_EQ(1024U, (audio_global + 1024 - ao + audio_random) - a);
	PASS();
}

TEST timeline_rates_wrap_and_long_uptime(void)
{
	static const uint32_t rates[] = {8000, 16000, 44100, 48000, 90000};
	const int64_t epoch = 5000000000000LL;
	for (unsigned int i = 0; i < sizeof(rates) / sizeof(rates[0]); ++i) {
		for (int64_t d = -1000000; d <= 1000000; d += 997) {
			uint32_t want = (uint32_t)(d * rates[i] / 1000000);
			ASSERT_EQ(want, rsd_timeline_ticks(epoch + d, epoch, rates[i]));
		}
		int64_t delta = 4000000000000LL;
		ASSERT_EQ((uint32_t)(delta * rates[i] / 1000000),
			rsd_timeline_ticks(epoch + delta, epoch, rates[i]));
	}
	ASSERT_EQ(0U, rsd_timeline_ticks(epoch, epoch, 90000));
	PASS();
}

SUITE(timeline_suite)
{
	RUN_TEST(timeline_keeps_initial_av_offset);
	RUN_TEST(timeline_rates_wrap_and_long_uptime);
}
