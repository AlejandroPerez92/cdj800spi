# CDJ-800MK2 MIDI controller with Arduino Uno

`MidiController.ino` turns panel buttons, pitch and jog movement into MIDI 1.0.
It targets **Uno R3 / ATmega328P**, using the hardware UART at **31250 baud**
for DIN MIDI OUT or the Uno's own USB with HIDUINO on its ATmega16U2/8U2.
No additional Arduino library is required.

This example was generated with AI from the recovered firmware protocol and
has not been tested on real hardware. Please
[report your test results](https://github.com/AlejandroPerez92/cdj800spi/issues/new?template=hardware-test.md).

## Uploading the example

In Arduino IDE, install CDJ800SPI and open
`File > Examples > CDJ800SPI > MidiController`. Select Arduino Uno and upload.
Keep `MidiConfig.h` and `MidiOutput.h` in the example folder.

From the PlatformIO project:

```sh
pio run -e uno_midi
pio run -e uno_midi -t upload
```

The `uno` environment still builds `PanelDemo` with its diagnostic console.

## Wiring

Disconnect MAIN at CN301 and keep the panel's own power supply. Connect the
panel's CN501 to the Uno:

| CN501 | Uno | Signal |
|---|---|---|
| 1 | D13 | SCK |
| 3 | D12 | MISO |
| 5 | D10 | SS |
| 7 | D11 | MOSI |
| 14 | GND | Common ground |

For DIN MIDI, add a five-pin output with this 5 V circuit. USB MIDI through
HIDUINO does not require this connector or these resistors:

```text
Uno 5V    ── 220 Ω resistor ── DIN MIDI OUT pin 4
Uno D1/TX ── 220 Ω resistor ── DIN MIDI OUT pin 5
Uno GND   ─────────────────── DIN MIDI OUT pin 2
                              pins 1 and 3 unconnected
```

Use the **connector's pin numbers**: the solder-side view mirrors the front
view. Connect MIDI OUT to MIDI IN on a USB–MIDI interface or the equipment you
want to control. D0/RX is unused by this example. This follows the
[Arduino MIDI example](https://docs.arduino.cc/built-in-examples/communication/Midi/)
and [DIN electrical specification](https://midi.org/5-pin-din-electrical-specs).

## Using the same USB connector as a MIDI device

**This is possible with an ATmega16U2 or ATmega8U2 USB chip.** That chip
normally acts as a [USB-to-serial converter](https://docs.arduino.cc/retired/boards/arduino-uno-rev3-with-long-pins/),
but [HIDUINO](https://github.com/ddiakopoulos/hiduino) can make it present a
standard USB MIDI device to the computer. The ATmega328P continues to handle
SPI and send MIDI messages over UART.

1. Upload `MidiController` with `pio run -e uno_midi -t upload` while the USB
   chip still runs its original serial firmware.
2. Keep `CDJ800_MIDI_BAUD=31250`, the rate used for
   [HIDUINO MIDI over USB](https://github.com/tttapa/MIDI_controller/blob/master/doc/MIDI-over-USB.md#hiduino-midi-over-usb).
3. Install HIDUINO on the **16U2/8U2 USB chip**, following its guide with the
   firmware/procedure for your exact chip. This uses DFU or an ISP programmer;
   it does not replace the ATmega328P sketch.
4. Reconnect USB, select the HIDUINO MIDI device in your DAW or DJ application,
   and apply this example's mapping.

HIDUINO disables the normal serial sketch-upload path. To update the sketch,
restore the USB-to-serial firmware or use an ISP programmer connected to the
ATmega328P. The [HIDUINO guide](https://github.com/ddiakopoulos/hiduino#quickstart)
describes both workflows.

To keep the original USB firmware, or for a board using CH340, CH341 or CP2102,
use a computer **serial-to-MIDI bridge** instead. Set `CDJ800_MIDI_BAUD` to
`115200` in `MidiConfig.h`, upload the sketch, and set the bridge to the same
rate. Select its virtual MIDI port in the application. These fixed-function
USB converters cannot run
[HIDUINO](https://github.com/tttapa/MIDI_controller/blob/master/doc/MIDI-over-USB.md#boards-with-a-single-purpose-usb-to-ttl-chip).
For DIN and HIDUINO, keep `31250`; changing the baud alone does not change the
USB device class.

The sketch sends binary data. Close the serial monitor while using a bridge
and do not add `Serial.print()`: text would corrupt the MIDI stream.

## Default mapping: channel 1

Buttons send **Note On**, velocity 127 on press, and **Note Off**, velocity 0
on release. Note numbers below are decimal. Octave labels differ between
applications, so map by number.

| Button | Note | Button | Note | Button | Note |
|---|---:|---|---:|---|---:|
| CUE | 36 | PLAY | 45 | QUICK RETURN | 54 |
| REV | 37 | SEARCH FWD | 46 | FOLDER FWD | 55 |
| CUE OUT | 38 | SEARCH REV | 47 | FOLDER REV | 56 |
| CUE IN | 39 | TRACK SEARCH FWD | 48 | TIME MODE | 57 |
| BEAT 8 | 40 | TRACK SEARCH REV | 49 | TEXT MODE | 58 |
| BEAT 4 | 41 | MASTER TEMPO | 50 | DELETE | 59 |
| BEAT 2 | 42 | TEMPO | 51 | MEMORY | 60 |
| BEAT 1 | 43 | VINYL | 52 | CALL REV | 61 |
| RELOOP | 44 | EJECT | 53 | CALL FWD | 62 |

| Input | Message | Value |
|---|---|---|
| Pitch slider | Absolute CC 20 | 0..127 |
| Jog rotation | Relative CC 22, seven-bit two's complement | +1 → 1; −1 → 127 |
| Raw `touch` byte, optional | Absolute CC 21 | 0..255 scaled to 0..127 |
| `jogPressure` bit, optional | CC 23 | 0 or 127 |

Enable the interface's MIDI input in your application and use MIDI Learn to
assign PLAY, CUE, loops and search. Select **relative / two's complement**
mode for CC 22; some applications label it `Relative (2's complement)`.
A frame can accumulate several steps: messages carry increments of up to +63
or −63, retaining any remainder for the next message. No value 64 or idle
message is sent. This relative encoding is an agreement with the application's
mapping, rather than a universal interpretation of any CC.

For example, pressing/releasing PLAY on channel 1 sends `90 2D 7F` / `80 2D 00`.
One positive jog step sends `B0 16 01`; one negative step sends `B0 16 7F`.
Note/CC formats follow the
[MIDI 1.0 message summary](https://midi.org/summary-of-midi-1-0-messages).

## Settings and panel feedback

Edit `MidiConfig.h` to change channel (1..16), first note, CC numbers,
pitch/jog orientation and sensitivity. Increase `JOG_PULSES_PER_STEP` to require
more pulses per MIDI step; the remainder is retained. Notes must stay within
0..127 and CC numbers within 0..119.

Pitch initially uses the full raw domain, `0..65535`. To use the full MIDI
resolution, upload `PanelDemo`, move the slider to both ends and record its
`pitch` values. Put the lower value in `PITCH_RAW_MIN` and the higher value in
`PITCH_RAW_MAX`; use `INVERT_PITCH` if needed. `pitchReference` is not used as a
center because its calibration meaning is unconfirmed. This CC is an absolute
MIDI Learn control; it represents neither a tempo percentage nor Pitch Bend.

`SEND_TOUCH` and `SEND_JOG_PRESSURE` default to disabled because their physical
meanings still need confirmation. Enable them after checking with `PanelDemo`.

The screen shows `MIDI CTRL`, the track field shows the channel, and the bar
follows pitch. PLAY toggles its light; CUE and other available indicators
follow button state, and the ring follows the jog. This feedback is local:
the example does not receive playback state or light commands from the computer.

The queue holds up to 63 button edges. UART transmission only occurs when all
three bytes fit. Pitch and optional inputs are sampled every 20 ms and sent
only when changed. Jog movement accumulates in fixed memory. If the queue
fills, all 27 mapped notes are explicitly released and currently held buttons
are restored; taps during recovery can be lost. After two seconds without
valid panel frames, all notes are released and pending movement is discarded.
Reconnection restores held buttons without turning the counter jump into
MIDI movement.

This example uses the same SPI transport as `PanelDemo`. The hardware UART
also generates interrupts: transmission without waits keeps `loop()` running,
but compilation and native tests do not verify timing on a physical panel.
