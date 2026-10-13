#pragma once
#include "StatusMessage.h"

struct Message { uint8_t content[BUFFER_LENGTH]; };

// select() returns the first byte; transfer() returns the NEXT byte to preload.
// deselect() closes one CS selection. RX usable queue capacity: two frames.
class SPIMessageHandler {
public:
    explicit SPIMessageHandler(uint8_t (*messages)[BUFFER_LENGTH]);
    void reset();
    bool publish();
    bool pending() const { return pendingBank_ != 0xff; }
    uint8_t select();
    uint8_t nextHeader() const { return group_ << 4; }
    // Already prefetched: write this to SPDR BEFORE receiveByte().
    uint8_t nextByteToSend() const { return nextToSend_; }
    void receiveByte(uint8_t received);
    uint8_t transfer(uint8_t received);
    void deselect();
    bool popMessage(Message &message);
    bool isMessageAvailable() const { return head_ != tail_; }
    uint8_t droppedMessages() const { return dropped_; }
    uint8_t framingErrors() const { return framingErrors_; }

private:
    uint8_t (*draft_)[BUFFER_LENGTH];
    uint8_t banks_[2][6][BUFFER_LENGTH];
    volatile uint8_t activeBank_, pendingBank_;
    Message queue_[3];
    uint8_t incoming_[BUFFER_LENGTH];
    volatile uint8_t head_, tail_, dropped_, framingErrors_;
    const uint8_t *txCursor_;
    uint8_t nextToSend_;
    uint8_t group_, count_;
    bool selected_;
    void pack(uint8_t bank);
};
