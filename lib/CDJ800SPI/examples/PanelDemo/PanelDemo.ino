#include <Arduino.h>
#include <CDJ800SPI.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>

CDJ800SPI cdj;
bool playing = false;
uint8_t jogPosition = 1;

// A fixed buffer lets Serial drain incrementally while loop() keeps reading SPI.
char consoleLine[224];
uint8_t consoleLength = 0, consoleOffset = 0;
uint32_t pendingPresses = 0, pendingReleases = 0;
int32_t pendingJogDelta = 0;
uint8_t previousRaw[19] = {};
bool inputsChanged = false, consoleStarted = false;
unsigned long lastInputLog = 0;

// Same order as Button. Store labels in flash to preserve Uno SRAM.
const char buttonNames[] PROGMEM =
    "CUE\0REV\0CUE_OUT\0CUE_IN\0BEAT8\0BEAT4\0BEAT2\0BEAT1\0RELOOP\0PLAY\0"
    "SEARCH_FWD\0SEARCH_REV\0TRACK_FWD\0TRACK_REV\0MASTER_TEMPO\0TEMPO\0VINYL\0"
    "EJECT\0QUICK_RETURN\0FOLDER_FWD\0FOLDER_REV\0TIME_MODE\0TEXT_MODE\0DELETE\0"
    "MEMORY\0CALL_REV\0CALL_FWD";

void collectInputs() {
    const StatusMessage &input = cdj.status();
    if (memcmp(previousRaw, input.raw, sizeof(previousRaw)) != 0) {
        memcpy(previousRaw, input.raw, sizeof(previousRaw));
        inputsChanged = true;
    }
    for (uint8_t i = 0; i < static_cast<uint8_t>(Button::Count); ++i) {
        const Button button = static_cast<Button>(i);
        if (cdj.pressed(button)) pendingPresses |= uint32_t(1) << i;
        if (cdj.released(button)) pendingReleases |= uint32_t(1) << i;
    }
    const int32_t delta = cdj.jogDelta();
    if (delta > 0 && pendingJogDelta > INT32_MAX - delta) pendingJogDelta = INT32_MAX;
    else if (delta < 0 && pendingJogDelta < INT32_MIN - delta) pendingJogDelta = INT32_MIN;
    else pendingJogDelta += delta;
}

void prepareButtonLog(uint8_t index, bool pressed) {
    const char *label = buttonNames;
    for (uint8_t i = 0; i < index; ++i)
        while (pgm_read_byte(label++) != 0) {}
    char name[16];
    uint8_t length = 0;
    while (static_cast<size_t>(length) + 1 < sizeof(name)) {
        const char value = pgm_read_byte(label++);
        if (value == '\0') break;
        name[length++] = value;
    }
    name[length] = '\0';
    snprintf_P(consoleLine, sizeof(consoleLine), PSTR("BTN %s %s\r\n"),
               name, pressed ? "pressed" : "released");
}

void serviceConsole() {
    if (consoleOffset == consoleLength) {
        consoleOffset = consoleLength = 0;
        if (!consoleStarted) {
            snprintf_P(consoleLine, sizeof(consoleLine),
                PSTR("CDJ800SPI ready: BTN events and INPUT values, 115200 baud\r\n"));
            consoleStarted = true;
        } else if (pendingPresses != 0 || pendingReleases != 0) {
            for (uint8_t i = 0; i < static_cast<uint8_t>(Button::Count); ++i) {
                const uint32_t mask = uint32_t(1) << i;
                if ((pendingPresses & mask) != 0) {
                    pendingPresses &= ~mask;
                    prepareButtonLog(i, true);
                    break;
                }
                if ((pendingReleases & mask) != 0) {
                    pendingReleases &= ~mask;
                    prepareButtonLog(i, false);
                    break;
                }
            }
        } else if (inputsChanged && millis() - lastInputLog >= 100) {
            const StatusMessage &input = cdj.status();
            snprintf_P(consoleLine, sizeof(consoleLine),
                PSTR("INPUT pitch=%u ref=%u touch=%u jog=%u interval=%u delta=%ld "
                     "flags=0x%02X aux13=0x%02X lost=%u invalid=%lu framing=%u raw="),
                unsigned(input.pitch), unsigned(input.pitchReference), unsigned(input.touch),
                unsigned(input.jogPulse), unsigned(input.jogSpeed), long(pendingJogDelta),
                unsigned(input.jogFlags), unsigned(input.auxiliary13), unsigned(cdj.droppedMessages()),
                static_cast<unsigned long>(cdj.invalidMessages()), unsigned(cdj.framingErrors()));
            uint8_t offset = strlen(consoleLine);
            const char hex[] = "0123456789ABCDEF";
            for (uint8_t i = 0; i < 20 && static_cast<size_t>(offset) + 4 < sizeof(consoleLine); ++i) {
                consoleLine[offset++] = hex[input.raw[i] >> 4];
                consoleLine[offset++] = hex[input.raw[i] & 15];
            }
            consoleLine[offset++] = '\r'; consoleLine[offset++] = '\n'; consoleLine[offset] = '\0';
            inputsChanged = false; pendingJogDelta = 0; lastInputLog = millis();
        } else return;
        consoleLength = strlen(consoleLine);
    }
    // Never block waiting for the serial TX buffer: RX has only two frame slots.
    const int available = Serial.availableForWrite();
    const uint8_t remaining = consoleLength - consoleOffset;
    const uint8_t count = available < remaining ? available : remaining;
    if (count != 0) {
        Serial.write(reinterpret_cast<const uint8_t *>(consoleLine + consoleOffset), count);
        consoleOffset += count;
    }
}

void setup() {
    Serial.begin(115200);
    cdj.setText("CDJ800SPI");
    cdj.setTrackNumber(1);
    cdj.setBpm(128);
    cdj.setTempo(0);
    cdj.setDisplaySeparator(on);
    cdj.setProgressPercentage(leftToRight, 50);
    const uint8_t marks[] = {1, 17, 33};
    cdj.setCueMarkers(marks, 3);
    cdj.setDisplayJogPos(jogPosition);
    if (!cdj.begin()) Serial.println(F("SPI slave unavailable or already in use"));
}

void loop() {
    if (cdj.update()) {
        collectInputs();
        if (cdj.pressed(Button::Play)) playing = !playing;
        cdj.setIndicator(Indicator::Play, playing);
        cdj.setIndicator(Indicator::Cue, cdj.status().btnCue);
        cdj.setIndicator(Indicator::Beat4, cdj.status().btnBeat4);
        int32_t position = int32_t(jogPosition) - 1 + cdj.jogDelta();
        position %= 135;
        if (position < 0) position += 135;
        jogPosition = static_cast<uint8_t>(position + 1);
        cdj.setDisplayJogPos(jogPosition);
        cdj.commit();
    }
    serviceConsole();
    // No delay: keep draining the two-frame receive queue.
}
