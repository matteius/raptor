# Bounded RTSP backpressure

The shared RTSP event loop previously waited on a per-client write mutex held
by the media sender, or wrote a response directly into a blocked socket.
A non-reading peer could therefore stall unrelated clients' control traffic.
This is independent of sensor, image tuning and media timestamp origin.

The repair defers a contended request without consuming its receive buffer,
retries it from the event loop, and closes the connection if the lock remains
busy for two seconds. Accepted TCP sockets have a two-second send timeout and,
where available, a five-second `TCP_USER_TIMEOUT`. Failed media/response writes
close the TCP connection: continuing after a partial interleaved packet would
corrupt framing. Background backchannel receiver reports skip a busy writer
without advancing the receiver-report history.

This is bounded blocking, not a completely asynchronous response writer.
TLS backend loops may retry `WANT_WRITE`; the TCP user timeout and deferred
request deadline bound the tested zero-window case. TLS-specific fault tests
have not been run, and this does not claim to fix every network outage.

## Evidence

Source `f431199` was built with the matching T41/uClibc toolchain. The source
revision comes from git, and only `rsd` was staged for the camera trial.
Binary SHA256:
`643a435ccbc5f9ddbbaf57fd20b552b745618c3573ae666340180511c46bddbf`.

- Raptor host ASan/UBSan suite: 347 tests, 29,329 assertions, all passing.
- Isolated host server with synthetic rings, 4 KiB send buffer and a
  non-reading 1 KiB receive-window peer: the old `227a5bf` server reproduced a
  healthy OPTIONS timeout at 3.003 seconds under syscall tracing. Untraced
  baseline trials sometimes passed, so the reproduction is schedule-dependent.
  Fixed traced and untraced trials return the healthy response in about 1 ms
  and disconnect the stalled peer. Full ASan/UBSan/LSan run is untraced;
  LeakSanitizer is disabled only under ptrace, which it does not support.
- Physical T41: six TCP/UDP H.264/AAC reconnects pass. With a deliberately
  stalled authenticated client, 20 independent healthy OPTIONS requests pass
  (maximum 0.555 s, mean 0.176 s). The server logs its deferred-request deadline
  and disconnects only the stalled peer. The existing VPN viewer is untouched.
- 120-second decode: exit 0, 2,969 video frames, exactly 120 seconds of output,
  no FFmpeg warnings or reported duplicate/drop events.
- 900-second decode: exit 0, 22,239 video frames, exactly 900 seconds of output,
  no FFmpeg warnings, read timeouts or reported duplicate/drop events.
- Separate RTP/RTCP check: no backward audio/video timestamps; estimated
  RTP-Info/SR origin deviations within 1.5 ms of the first video report.

The 900-second result is a clean **decoder/read-timeout** check, not a claim
of lossless source delivery. Camera logs still record one send-queue overflow
on each of two clients, with 20 video frames dropped per client. Delivered
rate is 24.72 fps at configured 25 fps. Queue/cadence efficiency remains a
separate investigation. The earlier read-timeout soak is not retroactively
counted as a pass, nor proven to have had only this cause.

No firmware image, sensor calibration, saved Raptor configuration, Neo audio
library, ISP module or other Raptor executable was changed by this trial.
Detailed local artifacts are in the task workspace's `iq/followup/rtsp-slow/`.
