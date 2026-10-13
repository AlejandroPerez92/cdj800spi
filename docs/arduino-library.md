# E02 panel integration with Arduino

The reusable driver is in [lib/CDJ800SPI](../lib/CDJ800SPI), with
[API documentation](../lib/CDJ800SPI/README.md), Arduino/PlatformIO metadata and
the [PanelDemo](../lib/CDJ800SPI/examples/PanelDemo/PanelDemo.ino) and
[MidiController](../lib/CDJ800SPI/examples/MidiController/README.md) examples.
The original prototype has been integrated there. Headers in `include/`
preserve the original include paths and the main `Cdj`/`StatusMessage` names.

This Arduino reconstruction was generated with AI from analysis of the
original CDJ-800MK2 E02 firmware binaries. **It has not been tested on real
hardware.** Please [open an issue with your results](https://github.com/AlejandroPerez92/cdj800spi/issues/new?template=hardware-test.md)
if you test it. Software checks and AVR builds do not establish panel compatibility.

## Panel fields recovered from E02

Groups and bytes are zero-based. Fields are prepared before calculating the
output checksum. Firmware addresses and file offsets identify the source
fields; they are not Arduino memory addresses.

| API / data | Firmware field or table | SPI location |
|---|---|---|
| Nine text glyphs | File offset `0x4aff8`, 256 × 10 bytes | Groups 0..4, bytes 1..10 |
| Tempo format/sign | CPU word `0x30602154` | Group 1, bytes 11..12 |
| Tempo digits | CPU bytes `0x3060215a..5c` | Group 2, bytes 11..13 |
| BPM digits | CPU bytes `0x30602160..62` | Group 3, bytes 11..13 |
| Track/folder digits | CPU bytes `0x30602166..68` | Group 4, bytes 11..13 |
| Cue markers | File offset `0x4baf3`, indices 0..33 | Group 5, bytes 1..5; low two bits of byte 5 |
| Separator | CPU byte `0x306021da`, bit 2 | Group 5, byte 5, mask 4 |
| Progress bar | File offset `0x4b9f8`, indices 0..33 | Group 5, byte 5 bits 3..7, bytes 6..8, byte 9 bits 0..3 |
| Symbols / BEAT 1/2 | CPU byte `0x3060216d` | Group 5, byte 12 |
| Button / auxiliary lights | CPU byte `0x3060216e` | Group 5, byte 13 |
| Jog position/animation | CPU byte `0x306021e3` | Byte 17 in every group |
| TX checksum/terminator | Sum of bytes 1..17 modulo 256 | Byte 18; byte 19 is zero |
| RX checksum | End-around-carry sum of bytes 0..18 | Byte 19 |
| Raw input | Complete received frame | Twenty-byte snapshot |

Bar patterns 0..33 contain 33 useful bits. Pattern 34 repeats 33; subsequent
bytes belong to other tables, so additional positions are not exposed.
Markers 0..33 use only their original table bits. Group 5 byte 13 bit 7 has
no confirmed physical identity and is named `Indicator::Auxiliary7`.

## Transport and concurrency

`SPIMessageHandler` maintains two banks of 6 × 20 bytes. The application edits
a separate draft. `commit()` copies it to the free bank and publishes its index
with an atomic byte write. The ISR activates that bank only at group 0. A
pending bank cannot be overwritten. All six groups retain one coherent
snapshot, including checksums, throughout a cycle.

The RX queue uses three fixed slots with an effective capacity of two frames.
Read/write indices have separate owners; the consumer releases a slot only
after copying it. Overflow discards the new frame and increments a counter
that saturates at 255. Checksum validation and parsing occur outside the ISR;
the complete incoming frame is copied when SS rises.

The AVR adapter uses mode 3 and watches SS through PCINT0. It preloads byte 0,
loads each next response into SPDR before processing RX, and drains a pending
last transfer if the SS interrupt runs before the SPI interrupt. Short and
overlong selections are discarded and restart TX at group 0. This is the
driver's recovery policy, rather than a claim about the original FPGA's
response to malformed packets. Queue/group rotation avoids divisions in the ISR.

The code avoids dynamic memory allocation and leaves SRAM available for the
Uno examples. Real operation requires measuring clock and byte/selection gaps.
Bar/marker orientation and provisional field names also need physical confirmation.

## Reproducible software checks

The default tests need Python 3 and Clang, and run without firmware binaries
or decompilation files:

```sh
python3 test/test_arduino_library.py -v
python3 test/test_midi_example.py -v
pio run -e uno -e mega2560 -e uno_midi
python3 tools/package_library.py
```

The native tests compile the actual portable Arduino library with ASan/UBSan.
They check known display/checksum/frame vectors, input mapping, truncated and
overlong selections, group wrap, TX-before-RX prefetch, coherent publication,
button edges, RX queue overflow and jog rollover. MIDI checks compile the real
sketch and replace only UART and clock. They cover all button messages,
simultaneous edges, congestion, analog coalescing, relative jog, saturation,
disconnect/reconnect, and calibrated/inverted configurations on channel 16.

Literal flash tables are included in the library, so building requires no
original firmware. To verify/regenerate them from your own extracted E02 image:

```sh
python3 tools/generate_panel_assets.py --firmware /path/to/firmware.bin --check
```

The generator requires SHA-256
`f32e594edaea183dce08023929aeb760b30dc8656accb35fa806d5e9137d96c7`
before using offsets. Remove `--check` to regenerate `PanelAssets.cpp`.
Original binaries and general reverse-engineering material are not part of
the Arduino package. Its ZIP contains only the library, tables, examples,
documentation, metadata and license.

## AVR builds

If PlatformIO's AVR compiler cannot execute on your host architecture, run
`sh tools/build_arduino_docker.sh`. It uses Linux `python:3.11-slim`, PlatformIO
6.2.0, Atmel AVR 5.3.0 and Arduino AVR 5.4.0, with a Docker volume for the tool
cache. HEX files are written to
`.pio/build-linux/{uno,mega2560,uno_midi}/firmware.hex`.

| Example / target | Static SRAM | Flash |
|---|---|---|
| PanelDemo / Uno ATmega328P | 1098 / 2048 bytes | 12080 / 32256 bytes |
| PanelDemo / Mega ATmega2560 | 1098 / 8192 bytes | 12576 / 253952 bytes |
| MidiController / Uno ATmega328P | 904 / 2048 bytes | 10180 / 32256 bytes |

The stack requires additional runtime memory. These are build results, not
hardware test results. `PanelDemo` logs named button edges and up to ten input
summaries per second using fixed buffers and available UART capacity.

The `uno_midi` environment selects `src/midi.cpp`, which includes the same
MIDI sketch distributed for Arduino IDE. Existing environments select
`src/main.cpp`. MIDI uses the hardware UART at 31250 baud for DIN or the Uno's
USB with HIDUINO on its ATmega16U2/8U2. Touch/pressure are disabled options
until their meanings are confirmed. Display and light feedback is local.

## Timing limits

The service manual lists TSCLK at **1 MHz**. Actual pauses between bytes and
selections are unknown. The current Uno driver does not sustain an uninterrupted
byte every 8 µs, and relies on the panel leaving gaps. Prioritizing SPDR writes
before RX processing restores the original prototype's transmission order.
The new driver still performs additional framing and queue work.

UART and Timer0 interrupts can delay SPI even though the examples avoid waits
in `loop()`. Native tests do not model that latency. Compatibility, timing,
physical orientation and provisional input/indicator identities remain
unverified until someone tests this implementation on a physical panel.
