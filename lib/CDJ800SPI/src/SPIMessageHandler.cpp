#include "SPIMessageHandler.h"
#include <string.h>

namespace {
inline void barrier() { __asm__ __volatile__("" ::: "memory"); }
void increment(volatile uint8_t &value) { if (value != 255) ++value; }
}

SPIMessageHandler::SPIMessageHandler(uint8_t (*messages)[BUFFER_LENGTH]) : draft_(messages) {
    reset();
}
void SPIMessageHandler::pack(uint8_t bank) {
    for (uint8_t group = 0; group < 6; ++group) {
        uint8_t *frame = banks_[bank][group];
        memcpy(frame, draft_[group], BUFFER_LENGTH);
        frame[0] = group << 4;
        uint8_t sum = 0;
        for (uint8_t i = 1; i <= 17; ++i) sum += frame[i];
        frame[18] = sum; frame[19] = 0;
    }
}
void SPIMessageHandler::reset() {
    activeBank_ = 0; pendingBank_ = 0xff;
    head_ = tail_ = dropped_ = framingErrors_ = 0;
    group_ = count_ = 0; selected_ = false;
    txCursor_ = nullptr; nextToSend_ = 0;
    pack(0); pack(1);
}
bool SPIMessageHandler::publish() {
    if (pendingBank_ != 0xff) return false;
    const uint8_t bank = activeBank_ ^ 1;
    pack(bank);
    barrier();
    pendingBank_ = bank; // atomic byte publication on AVR
    return true;
}
uint8_t SPIMessageHandler::select() {
    if (selected_) deselect();
    selected_ = true; count_ = 0;
    if (group_ == 0 && pendingBank_ != 0xff) {
        activeBank_ = pendingBank_;
        barrier();
        pendingBank_ = 0xff;
    }
    const uint8_t *frame = banks_[activeBank_][group_];
    nextToSend_ = frame[1];
    txCursor_ = frame + 2;
    return frame[0];
}
void SPIMessageHandler::receiveByte(uint8_t received) {
    if (!selected_) return;
    const uint8_t count = count_;
    if (count < BUFFER_LENGTH) incoming_[count] = received;
    if (count < 21) count_ = count + 1;
    // Compute the following response AFTER the current response was loaded.
    // Never read past byte19, including an overlong CS selection.
    nextToSend_ = count < 18 ? *txCursor_++ : 0;
}
uint8_t SPIMessageHandler::transfer(uint8_t received) {
    const uint8_t next = nextToSend_;
    receiveByte(received);
    return next;
}
void SPIMessageHandler::deselect() {
    if (!selected_) return;
    selected_ = false;
    nextToSend_ = 0;
    if (count_ == BUFFER_LENGTH) {
        uint8_t next = head_ + 1;
        if (next == 3) next = 0;
        if (next == tail_) increment(dropped_);
        else {
            memcpy(queue_[head_].content, incoming_, BUFFER_LENGTH);
            barrier(); head_ = next;
        }
        if (++group_ == 6) group_ = 0;
    } else {
        increment(framingErrors_);
        group_ = 0; // deterministic restart after a broken transaction
    }
    count_ = 0;
}
bool SPIMessageHandler::popMessage(Message &message) {
    const uint8_t tail = tail_;
    if (head_ == tail) return false;
    barrier();
    memcpy(message.content, queue_[tail].content, BUFFER_LENGTH);
    uint8_t next = tail + 1;
    if (next == 3) next = 0;
    barrier(); tail_ = next;
    return true;
}
