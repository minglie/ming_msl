#include "UInt32FIFO.h"
#include <iostream>

UInt32FIFO::UInt32FIFO()
        : head(0), tail(0), count(0) {}

bool UInt32FIFO::push(uint32_t value) {
    if (isFull()) return false;
    buffer[tail] = value;
    tail = (tail + 1) % Capacity;
    ++count;
    return true;
}

bool UInt32FIFO::pop(uint32_t &value) {
    if (isEmpty()) return false;
    value = buffer[head];
    head = (head + 1) % Capacity;
    --count;
    return true;
}

bool UInt32FIFO::isEmpty() const {
    return count == 0;
}

bool UInt32FIFO::isFull() const {
    return count == Capacity;
}

size_t UInt32FIFO::size() const {
    return count;
}

void UInt32FIFO::debugPrint() const {
    std::cout << "FIFO [ ";
    for (size_t i = 0, idx = head; i < count; ++i, idx = (idx + 1) % Capacity) {
        std::cout << buffer[idx] << ' ';
    }
    std::cout << "]" << std::endl;
}
