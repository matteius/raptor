# Shared RTSP media origin

Each PLAY response declares an RTP-Info base for audio and video. These bases
must denote the same NPT instant, not each track's independently arriving first
sample. See [RTSP 1.0 section 12.33](https://www.rfc-editor.org/rfc/rfc2326#section-12.33).
RTCP subsequently relates each wire clock to a common time reference; its RTP
and NTP fields describe one instant. See [RTP section 6.4.1](https://www.rfc-editor.org/rfc/rfc3550#section-6.4.1).

The previous reader subtracted the first global RTP timestamp independently
for each track. This made both initial samples appear at NPT zero even when
their capture instants differed. With 16 kHz AAC, a capture offset larger than
one 1024-sample frame could produce backward receiver PTS at initial RTCP
synchronization, despite monotonically increasing raw RTP.

On a T41/OS04D10 test device, FFmpeg's audio PTS went 15360 -> 15269 and a
debug repeat went 15360 -> 15350. This was distinct from video null-sink
time-base quantization. A separate raw capture had no RTP reversal but its
independently based audio/video origins differed by about 45 ms.

The reader now computes each track's offset against the same per-client
CLOCK_MONOTONIC origin established at PLAY. It uses the existing mapped video
capture timestamp and the audio ring capture timestamp, preserving initial
A/V phase. Normal RTP cadence, sender-report scheduling, random wire bases,
keyframe gating and monotonic recovery guards are unchanged. Negative preroll
and RTP wrap are handled modulo 2^32. No device-specific delay is added.

Host tests cover unequal first-sample times, negative preroll, wrap, long
uptime and 8/16/44.1/48/90 kHz clocks. All 344 project tests pass under the
project's ASan/UBSan configuration. The T41 build passes. The one-shot device
candidate passed six TCP debug-timestamp sessions plus three TCP and three
UDP decode/reconnect sessions, without backward PTS or timestamp warnings.
This does not establish arbitrary-client, PAUSE/resume or long-term clock
drift coverage; those remain separate regression scenarios.
