# CDJ800SPI

An Arduino library replacing the MAIN/FPGA end of the Pioneer CDJ-800MK2 panel
SPI link. The panel is the **master**; Arduino is the **slave**, mode 3,
MSB first. The CDJ-800 MK1 panel is not compatible.

The Arduino driver, examples, documentation and tests were generated with AI
from analysis of the original E02 firmware binaries. **This implementation
has not been tested on real hardware.** If you try it, please
[open an issue with your results](https://github.com/AlejandroPerez92/cdj800spi/issues/new?template=hardware-test.md),
including your setup, working/failing controls and available logs or captures.

The library controls nine-character text, track/folder number, BPM, tempo,
indicators, a 33-segment bar, ten markers and the jog display. It receives all
27 buttons, pitch, a reference value, jog counter/interval and the raw frame.
It needs neither ArduinoQueue nor dynamic memory allocation.

The service manual specifies **TSCLK at 1 MHz**. The current Uno transport
requires gaps between bytes and selections; it cannot sustain a continuous
1 MHz stream. Compilation and software tests do not verify physical timing.

## Installation and wiring

Copy this folder into `Arduino/libraries/CDJ800SPI`, or install the generated
ZIP with `Sketch > Include Library > Add .ZIP Library`. Open
`File > Examples > CDJ800SPI > PanelDemo`. PlatformIO automatically finds the
library in the project's `lib/` directory.

`PanelDemo` also prints inputs at **115200 baud**:

```text
BTN PLAY pressed
BTN PLAY released
INPUT pitch=32768 ref=32960 touch=0 jog=135 interval=240 delta=1 flags=0xC0 aux13=0x00 lost=0 invalid=0 framing=0 raw=...
```

Button names appear on press and release. Analog and jog values print when
changed, at most ten times per second, with the full hexadecimal frame and
error counters. `delta` accumulates movement since the last summary. Open the
Arduino IDE serial monitor or run `pio device monitor -b 115200`. Values are
raw: pitch is not converted to a percentage, and the jog interval has no
established physical units.

The example sends each line incrementally using `Serial.availableForWrite()`
so it can keep servicing SPI. Pending events are grouped by button: a pending
press and release are both printed, but repeated taps of the same button
while the console drains may be coalesced.

[MidiController](examples/MidiController/README.md) is an Uno example with
notes for all 27 buttons, pitch CC 20, relative jog CC 22 and local display/light
feedback. It uses the UART at 31250 baud for DIN MIDI or the Uno's USB with
HIDUINO on its ATmega16U2/8U2 chip. The guide covers wiring, MIDI mapping,
calibration and a computer serial-to-MIDI bridge alternative. Open
`File > Examples > CDJ800SPI > MidiController`, or build `pio run -e uno_midi`
from the repository.

The adapter supports classic Uno/Nano ATmega328P/168 and Mega ATmega2560/1280.
It does not apply to Uno R4, Nano Every or other microcontrollers.

Disconnect MAIN at CN301 before connecting Arduino. Keep the panel's own
power supply; these five wires connect only signals and ground.

| CN501 DFLB | Signal | Uno / classic Nano | Mega |
|---|---|---|---|
| 1 | SCK | 13 | 52 |
| 3 | MISO, Arduino → panel | 12 | 50 |
| 5 | SS, active low | 10 | 53 |
| 7 | MOSI, panel → Arduino | 11 | 51 |
| 14 | GND | GND | GND |

The panel link uses 5 V logic. Ports to 3.3 V boards require level conversion
and a suitable hardware SPI slave peripheral.

## Usage

```cpp
#include <CDJ800SPI.h>
CDJ800SPI cdj;

void setup() {
    cdj.setText("HELLO CDJ");
    cdj.setTrackNumber(7);
    cdj.setBpm(128);
    cdj.setTempo(-250);                 // -2.50 %
    cdj.setDisplaySeparator(on);
    cdj.setProgressPercentage(leftToRight, 50);
    cdj.setDisplayJogPos(33);
    cdj.begin();
}

void loop() {
    if (cdj.update()) {
        cdj.setIndicator(Indicator::Cue, cdj.status().btnCue);
        if (cdj.pressed(Button::Play)) {
            // Handle the PLAY press here.
        }
        cdj.commit();
    }
}
```

`begin()` loads prepared output and configures SPI/SS. It returns `false` if
another object owns the transport or the board is unsupported. It does not
start Serial. Setters may be called before or after `begin()`.

`update()` drains at most two pending frames, updates `status()`, accumulates
edges and jog delta, and publishes display changes. It returns `true` if any
valid frame was accepted. Call it continuously without long waits. `pressed()`,
`released()` and `jogDelta()` apply to that call and are cleared on the next
call. If a press and release arrive together, both flags are true and
`status()` contains the final state. The first packet establishes the jog
reference; buttons already held generate press events.

`commit()` publishes a snapshot of all six frames. It becomes active when
group 0 is selected; the previous snapshot is retained for the rest of the
current cycle. If another publication is pending, it returns `false`: changes
remain in the draft and `update()` retries automatically. Do not spin waiting
for completion: the panel generates the clock. `displayPending()` includes
both draft changes and pending publications.

## Display and lights

| API | Values / behavior |
|---|---|
| `setText(text)` | Nine glyph indices; pads with spaces and truncates. `nullptr` clears text. Uses ASCII/firmware bytes, without UTF-8 conversion. |
| `setGlyphs(indices)` | Exactly nine bytes, indices 0..255; supports the time-format glyph `0x80`. |
| `setTrackNumber(number)` | 0..999, without leading zeros; returns `false` out of range. |
| `setBpm(bpm)` | Integer; clamps to 399, without leading zeros. |
| `setTempo(hundredths, wide, masterTempo)` | Hundredths of a percent. Normal ±1600; WIDE ±10000. Reproduces the firmware's sign and variable precision. Rejects out-of-range values without changes. Also updates WIDE/MASTER TEMPO. |
| `setIndicator(indicator, enabled)` | PLAY, CUE, Reverse, CueIn/Out, Beat1/2/4/8, Wide, MasterTempo, Vinyl, QuickReturn and Auxiliary7. |
| `setProgressPercentage(direction, percentage)` | 0..100, clamps to 100; replaces the old pattern, including when decreasing. |
| `setProgressSegments(count, direction)` | 0..33. Uses the original table; reverse direction complements the draft pattern. |
| `setCueMarkers(positions, count)` | Up to ten positions 1..33; 0 means empty. `(nullptr, 0)` clears markers. Preserves the bar and separator. |
| `setDisplaySeparator(on/off)` | Preserves markers and bar. |
| `setDisplayJogPos(position)` | Clamps to 1..135 and copies the position to all six groups. |
| `setJogMode(mode)` | `Blank`, `Full`, `Load`, `Eject`: codes 0/136/137/138. |
| `clearDisplay()` | Clears all output fields. Also provides `clearBpm()`, `clearTempo()`, `clearTrackNumber()` and `clearDisplayJog()`. |

The original bar has **33** useful positions. The prototype's 37-position
calculation was replaced with E02 patterns. Direction names remain API
conventions; physical orientation needs a capture. `Auxiliary7` exposes bit 7
of group 5 byte 13, whose physical purpose is unidentified. BEAT 1/2 and
animation names follow the analysis/prototype and need hardware confirmation.

`setRawByte(group, index, value)` accesses data bytes 1..16 in groups 0..5.
It cannot modify headers, checksums, terminators or the shared jog field.
`getFrame(group, output)` reads the twenty prepared bytes with their checksum;
it does not guarantee that they are already active on SPI. Raw operations
allow experiments with unidentified symbols and fields. Text setters replace
the matrix bytes; apply raw overlays afterwards to preserve them.

## Inputs

`status().raw[20]` retains the complete last valid frame. Its fields include:

| Field | Bytes / interpretation |
|---|---|
| `touch` | Byte 3; inherited name, physical purpose unconfirmed. |
| `pitch` | BE16(4); raw sample. |
| `pitchReference` | BE16(6); probable reference/calibration value. |
| `jogPulse` | BE16(8); circular 16-bit counter. |
| `jogSpeed` | BE16(10); inherited name, apparently an interval, with no established units. |
| `jogFlags` | Complete byte 12; bits 0x80/0x20 corroborated in the firmware. |
| `jogForward`, `jogPressure` | Inherited bits 0x40/0x10; meanings need confirmation. |
| `auxiliary13` | Byte 13, with no proven physical identity. |
| `btn...`, `down(Button::...)` | Buttons using the prototype's map. |

Jog delta comes from the counter without using `jogForward`. It preserves the
original `>= 0x7fff` threshold, including exceptional differences of ±32769.
The library does not apply pitch filtering, debounce or player behavior.

`receivedMessages()` and `invalidMessages()` use 32-bit counters.
`droppedMessages()` and `framingErrors()` saturate at 255. RX keeps two pending
frames and discards new arrivals when full; the counter indicates lost events.
Invalid frames neither update `status()` nor create events.

## SPI and interrupt contract

Each SS selection contains exactly 20 bytes. Output starts with group 0,
cycles through 0..5 and returns to 0. Keeping SS low does not concatenate
frames: bytes after byte 19 return zeros. A short or long selection is
discarded in RX and restarts TX at group 0, as a driver recovery policy.

Output is `[group<<4, data1..16, jog, sum(data1..17)&255, 0]`.
Input requires header `01 10`; its checksum sums bytes 0..18 with end-around
carry and no inversion. A positive sum divisible by 255 produces **0xff**.

The library installs `SPI_STC_vect` and `PCINT0_vect`. Do not declare another
SPI ISR in the sketch. If sharing the PORTB change interrupt, define
`CDJ800SPI_EXTERNAL_INTERRUPTS` for the whole build and dispatch to
`Cdj::spiInterrupt()` / `Cdj::selectInterrupt()` from your ISRs.
Do not call setters, `commit()` or `update()` from interrupts. The AVR adapter
loads a prefetched response into SPDR before processing RX. Queue/group
rotation uses no divisions in the ISR.

For another slave HAL, preload `onSelect()` when SS falls; pass completed MOSI
bytes to `onTransfer()` and preload its result; call `onDeselect()` when SS
rises. Publication uses atomic byte operations and compiler barriers for one
ISR producer and one main-loop consumer on AVR. A multicore/threaded port
must provide its own synchronization.

SS transitions and bytes require sufficient time for their ISRs. The nominal
clock is known but real gaps are not. UART and Timer0 can delay SPI even when
logging does not wait in `loop()`. Software tests do not prove these physical
margins; see the repository's `docs/arduino-library.md` for integration notes.

## Provenance and software verification

The implementation follows fields recovered from the extracted E02 image,
SHA-256 `f32e594edaea183dce08023929aeb760b30dc8656accb35fa806d5e9137d96c7`.
Literal glyphs and patterns are stored in `PROGMEM`. `PanelAssets.cpp` records
their file offsets. These original data tables have separate provenance from
the generated driver source. The original firmware image is not required to
compile or run the software tests and is not distributed with the library.

From the repository root:

```sh
pio run -e uno -e mega2560 -e uno_midi
python3 test/test_arduino_library.py -v
python3 test/test_midi_example.py -v
```

The native tests compile the actual portable driver with AddressSanitizer and
UndefinedBehaviorSanitizer. They check known checksum/display/frame vectors,
input mappings, coherent publication, group wrap, truncation, overlong
selections, queue overflow, button edges and jog rollover. The MIDI tests also
exercise the real sketch with a simulated UART and clock.

If you have the exact extracted E02 image locally, the optional
`python3 tools/generate_panel_assets.py --firmware /path/to/firmware.bin --check`
checks the tables byte for byte against it. This requires your own image; it
is not part of the default test command.

Hardware timing, physical orientation and provisional identities still need
verification on a real CDJ. The library handles panel fields; it does not
implement the audio engine, DSP, servo or E02 operating system.
