# Appendix G. Audio constants and experiments

This appendix is mainly for anyone changing the audio board's firmware, or running the pop-hunt
experiments again. It lists the numbers compiled into the A32's audio and Bluetooth firmware, and the
experiments that can change its behaviour until the next restart. Chapter 10 explains what each one does. The stored
audio and Bluetooth settings, which a user can change and keep, are in Appendix A; every A32 console key
is in Appendix D.

## G.1 Compiled constants

| Name | Value | Unit | Meaning | Where |
|---|---|---|---|---|
| `SAMPLE_RATE` | 44 100 | Hz | Fixed; there is no resampler. | `audio.cpp` |
| `BLOCK` | 256 | frames | One pass of the audio task, about 5.8 ms. | `audio.cpp` |
| `RING_FRAMES` | 8192 | frames | The Bluetooth ring: about 186 ms, 32 KB. | `audio.cpp` |
| `dma_buf_count` / `dma_buf_len` | 8 / 256 | buffers / frames | 46.4 ms per direction, 32 KB for both directions. Do not raise without measuring (chapter 10, section 10.8). | `Audio::begin()` |
| MCLK | 11.2896 | MHz | 256 × 44 100, from the APLL. | `Audio::begin()` |
| Clock-pad drive at start-up | 3 | 0–3 | GPIO0, 18 and 17. The remote setting clamps to 2–3. | `Audio::begin()` |
| Data-pad drive | the power-on default | 0–3 | Never set at start-up. | — |
| `tx_sd_out_delay` | never written; read back only (`sddly`) | 0–3 | **Hazard:** see chapter 10, section 10.15.3. Nothing can write it. | `sdOutDelayIs()` |
| Audio task | core 1, priority 6, 4096 B stack | | | `Audio::begin()` |
| `ENV_ONE` | 2^24 | Q24 | Envelope unity. | `audio.cpp` |
| Volume-law clamp | 10–40 | gamma × 10 | Gamma 1.0 to 4.0. | `setTaper()` |
| Balance step | 655 | Q16 per unit | ±100 leaves about −65 dB. | `audioTask()` |
| `ZDD_FRAMES` | 1024 | frames | The DAC's zero-data window, 23.22 ms. | `audio.cpp` |
| `ZDD_LSB` | `0x00000100` | word | One 24-bit step, −138.5 dBFS: the zero-data floor's word. | `audio.cpp` |
| `ZDD_STALL_US` | 23 220 | µs | A gap between writes above this is a stall. | `audio.cpp` |
| `DEVICE_NAME` | "Ambersong" | | The Bluetooth name. | `main.cpp` |
| `BT_DROP_TIMEOUT_MS` | 3000 | ms | How long a disconnect may stay in flight; the spacing of stray-phone drops. | `main.cpp` |
| Pause-to-disconnect gap | 250 | ms | | `gracefulDisconnect()` |
| `BT_LOOK` → `BT_STDBY` | 20 000 | ms | | `updateBtState()` |
| `btTxLevel` | `ESP_PWR_LVL_N0` (0 dBm) | level 0–5 = −12 to +3 dBm | The Bluetooth transmit ceiling at start-up; the floor is 0 dBm. | `main.cpp` |
| `kDc[7]` | `0xFF000000`, `0x00FF0000`, `0xFFFF0000`, `0x00FFFFFF`, `0x000000FF`, `0x00100000`, `0x001FFFFF` | words | The constant-DC diagnostic's words, all at or below −42 dBFS. | `main.cpp` |
| DC word at start-up | `0xFF000000` | | The proven popper (Appendix H). | `audio.cpp` |
| Update mute wait | 400 | ms | Before the A32 starts writing its flash (chapter 3, section 3.10.3). | `onMessage()` |
| Update give-up | 20 000 | ms | No frame from the S3. | `loop()` |
| Source and knob reading | 50 / 50 | ms | | `loop()` |
| `POT_DEADBAND` | 24 | ADC counts | About 0.6 % of the knob's travel. | `main.cpp` |
| Selector thresholds | 424 / 2471 | raw ADC | Below 424 AUX; 424 to 2470 BT; 2471 and above RADIO. | `pins.h` |
| `PROTO_STATE_MS` | 250 | ms | The state message to the S3. | `proto.h` |
| `PROTO_SILENCE_MS` | 2000 | ms | The S3 is considered gone, and the A32 sleeps. | `proto.h` |
| Settings save delay | 2000 | ms | After the last change (chapter 7, section 7.4.2). | `settingsFlush()` |
| Pair button | counted on release, after more than 30 ms held | | Only on the BT source. | `loop()` |
| Blue lamp PWM | LEDC channel 0, 2 kHz, 8-bit | | | `btled.cpp` |
| Knob calibration fallback | 60 and 3990, no centre | raw ADC | Used when the stored maximum is not more than 64 counts above the minimum. | `potRotation()` |

## G.2 Experiments

Experiments are **actions, not settings**: none survives a restart of the A32. The reason:
"an experiment that survives a reboot is one whose result gets attributed to something else a week
later." Every one is read back from the hardware and shown on the portal (chapter 10, section 10.9).

**From the portal** (administrator; every portal action is in Appendix C):

| Portal action (message) | Effect on the A32 | Range | Notes |
|---|---|---|---|
| `sys.clkdrv` (`MSG_CLK_DRIVE`) | Drive of the clock pads GPIO0, 18 and 17 together. | clamped to 2–3 | The pop knob; start-up value 3. |
| `sys.clkpair` (`MSG_CLK_PAIR`) | MCLK drive, then BCK/LRCK drive. | 22, 23, 32 or 33 only; others refused | Never run on the radio. |
| `sys.dindrv` (`MSG_DIN_DRIVE`) | Data-line pad drive. | 0–3 | 0 and 1 may mis-decode for long stretches: use it with someone present and the amplifier down. |
| `sys.dctest` (`MSG_TEST_DC`) | Replace the output with the DC word. | 0/1 | Silent as audio; anything heard is the fault. |
| `sys.dcword` (`MSG_DC_WORD`) | Pick `kDc[index]`. | 0–6; others refused | An index, never a raw word, so nothing louder than −42 dBFS can be sent. |
| `sys.zfloor` (`MSG_ZERO_FLOOR`) | The zero-data floor (below). | 0/1 | |
| `sys.tail` (`MSG_SIGN_TAIL`) | The sign-extended tail: the last 8 bits of each word take the sign of the next word. | 0/1 | Ruled out as a pop fix. |
| `sys.pot` (`MSG_USE_POT`) | Ignore the front knob (0) or use it (1). | 0/1 | |
| `sys.bttx` (`MSG_BT_TX`) | The Bluetooth transmit ceiling. | 0–5 (−12 to +3 dBm); others refused | Down only; 5 restores the stock range. |
| retired `MSG_ADC_CLOCK` | Ignored. | | The ADC master-clock toggle, removed. |
| retired `MSG_SD_DELAY` | Ignored. | | The data-line output delay, removed. |

**From the A32's USB console only** (115 200 baud, on the A32's own cable):

| Key | Effect |
|---|---|
| `d` | Cycle the DC word. The first press selects `kDc[1]` (`0x00FF0000`), because the start-up word is `kDc[0]`. |
| `k` | Cycle the low-bit output mask: 0, 1, 2, 4, 6, 8, 12, 16 bits. Not listed in the start-up help line. |
| `g` | Cycle the data-line drive 0–3. Not listed in the start-up help line. Same hazard as `sys.dindrv`. |
| `v` | The knob on or off. |
| `r` | A raw knob stream at 10 Hz. |

The key `y`, which cycled `tx_sd_out_delay`, has been removed.

**The diagnostics, in the order the audio task applies them** (all off by default):

| Diagnostic | What it does | What it was for |
|---|---|---|
| Constant DC word | Replaces every sample with a fixed word, silent as audio. | Anything heard is the fault. `0xFF000000` is the proven popper and `0x00FF0000` its matched silent control (Appendix H). |
| Low-bit output mask | Zeroes the bottom bits of each word. | Keep the data line still at the word boundary. Never confirmed by ear. |
| Zero-data floor | Keeps the DAC's output stage alive on a constant word, `ZDD_LSB` (one 24-bit step, −138.5 dBFS). | Keep the DAC's own zero-data mute off, on the theory that its release made the pop. Excluded (Appendix H). |
| Sign-extended tail | Sets the last 8 bits of each word to the sign of the next word. | Stop the data line moving across a word boundary. With it on, a word of 1 to 255 followed by a non-negative word becomes exact zero. |
