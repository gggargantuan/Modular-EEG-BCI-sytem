# ADS131M08 → LSL Streaming Bridge

Firmware + Python bridge for streaming 8-channel data from a Texas Instruments
ADS131M08 ADC (via Arduino) into Lab Streaming Layer (LSL) as an 8-channel
`EEG` stream.

## Repo contents

| File | Role |
|---|---|
| `data_streaming_binary.ino` | Base Arduino firmware. Reads all 8 ADC channels each DRDY pulse and streams them over serial as a packed binary struct. No per-channel polarity modification. |
| `data_streaming_binary_inverted-8th-ch.ino` | Same firmware, **with channel 8 (index 7) polarity inverted** in software (`pkt.ch[7] = -pkt.ch[7];`) before transmission. Use this variant only if channel 8 is known to be wired/recorded with reversed polarity relative to the other 7 channels. |
| `LSL_streaming_binary_py_2.0.py` | Python-side serial → LSL bridge. Parses the binary packet, checks for dropped packets via the counter, and pushes samples to an LSL outlet. No inversion or per-channel logic — channels pass through unchanged, in order. |

## Serial packet format

38 bytes total, sent once per sample (`__attribute__((packed))` struct, so no
compiler padding):

| Bytes | Field | Type |
|---|---|---|
| 0 | `sync1` | `uint8_t` = `0xAA` |
| 1 | `sync2` | `uint8_t` = `0x55` |
| 2–5 | `counter` | `uint32_t`, little-endian, increments every sample, wraps at 2^32 |
| 6–37 | `ch[0..7]` | 8 × `int32_t`, little-endian, sign-extended from the ADC's native 24-bit output |

The Python side matches this with `struct.unpack_from('<I', payload, 0)` for
the counter and `struct.unpack_from('<8i', payload, 4)` for the 8 channels.


## How to run for visualisation in OpenViBE:

1. Upload firmware on microcontroler
2. Run LSL bridge 
3. Open OpenViBE Acquisition Server
4. Select LSL driver an open the Preferences menu and confirm (do even if nothing needs to be configured further)
5. Press Connect and then Play
6. Open OpenViBE Designer and play the chosen scenario