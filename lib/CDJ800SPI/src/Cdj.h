#pragma once
#include "SPIMessageHandler.h"

enum LED_STATUS { on, off };
enum COMMAND_TYPE { buttonStatus = 5 };
enum DIRECTION { rightToLeft, leftToRight };
enum class Indicator : uint8_t {
    Play, Cue, Reverse, CueIn, CueOut, Beat8, Beat4, Beat2, Beat1,
    Wide, MasterTempo, Vinyl, QuickReturn, Auxiliary7, Count
};
enum class JogMode : uint8_t { Blank = 0, Full = 136, Load = 137, Eject = 138 };

class Cdj {
public:
    Cdj();
    ~Cdj() { end(); }
    Cdj(const Cdj &) = delete;
    Cdj &operator=(const Cdj &) = delete;
    bool begin(); // Uno/Nano ATmega328P/168, Mega ATmega2560/1280
    void init() { begin(); }
    void end();
    bool update(); // drains RX, collects edges, publishes changed display
    bool commit(); // false while a previous publication awaits group zero
    bool displayPending() const { return dirty_ || handler_.pending(); }
    const StatusMessage &status() const { return status_; }
    StatusMessage lastStatus() { update(); return status_; }
    bool hasStatus() const { return hasStatus_; }
    bool pressed(Button button) const;
    bool released(Button button) const;
    int32_t jogDelta() const { return jogDelta_; }
    static int32_t jogDifference(uint16_t current, uint16_t previous);
    uint32_t receivedMessages() const { return received_; }
    uint32_t invalidMessages() const { return invalid_; }
    uint8_t droppedMessages() const { return handler_.droppedMessages(); }
    uint8_t framingErrors() const { return handler_.framingErrors(); }

    void clearDisplay();
    void setText(const char *text); // pads/truncates to nine firmware glyph indices
    void setGlyphs(const uint8_t glyphs[9]);
    bool setTrackNumber(uint16_t track); // 0..999, blank leading zeroes
    void clearTrackNumber();
    void setBpm(uint16_t bpm); // saturates at 399
    void clearBpm();
    bool setTempo(int16_t hundredths, bool wide = false, bool masterTempo = false);
    void clearTempo();
    void setIndicator(Indicator indicator, bool enabled);
    void setJogMode(JogMode mode);
    void setDisplayJogPos(uint8_t position);
    void setEjectAnimation() { setJogMode(JogMode::Eject); }
    void setLoadInAnimation() { setJogMode(JogMode::Load); }
    void setDisplayFullJog() { setJogMode(JogMode::Full); }
    void clearDisplayJog() { setJogMode(JogMode::Blank); }
    void setDisplaySeparator(LED_STATUS state);
    void setProgressPercentage(DIRECTION direction, uint8_t percentage);
    bool setProgressSegments(uint8_t segments, DIRECTION direction = leftToRight);
    bool setCueMarkers(const uint8_t *positions, uint8_t count); // ten slots, 0..33
    bool setRawByte(uint8_t group, uint8_t index, uint8_t value); // data bytes1..16
    bool getFrame(uint8_t group, uint8_t output[20]) const; // staged frame + checksum

    void ledPlaySetStatus(LED_STATUS s) { setIndicator(Indicator::Play, s == on); }
    void ledCueSetStatus(LED_STATUS s) { setIndicator(Indicator::Cue, s == on); }
    void ledRevSetStatus(LED_STATUS s) { setIndicator(Indicator::Reverse, s == on); }
    void ledCueInSetStatus(LED_STATUS s) { setIndicator(Indicator::CueIn, s == on); }
    void ledCueOutSetStatus(LED_STATUS s) { setIndicator(Indicator::CueOut, s == on); }
    void ledBeat8SetStatus(LED_STATUS s) { setIndicator(Indicator::Beat8, s == on); }
    void ledBeat4SetStatus(LED_STATUS s) { setIndicator(Indicator::Beat4, s == on); }
    void ledBeat2SetStatus(LED_STATUS s) { setIndicator(Indicator::Beat2, s == on); }
    void ledBeat1SetStatus(LED_STATUS s) { setIndicator(Indicator::Beat1, s == on); }

    // Portable transport hooks; call from a board-specific slave ISR only.
    uint8_t onSelect() { return handler_.select(); }
    uint8_t onTransfer(uint8_t received) { return handler_.transfer(received); }
    void onDeselect() { handler_.deselect(); }
    void handleSPI();
    void handleSelect();
    static void spiInterrupt();
    static void selectInterrupt();

private:
    uint8_t commands_[6][BUFFER_LENGTH];
    SPIMessageHandler handler_;
    StatusMessage status_;
    bool dirty_, hasStatus_, hardwareSelected_;
    uint32_t pressed_, released_, received_, invalid_;
    int32_t jogDelta_;
    uint16_t previousJog_;
    static Cdj *volatile instance_;
    void write(uint8_t group, uint8_t index, uint8_t value);
};
