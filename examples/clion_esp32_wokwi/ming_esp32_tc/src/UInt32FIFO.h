#ifndef UINT32FIFO_H
#define UINT32FIFO_H

#include <cstdint>
#include <cstddef>

class UInt32FIFO {
public:
    static constexpr size_t Capacity = 16;

    UInt32FIFO();

    bool push(uint32_t value);
    bool pop(uint32_t &value);
    bool isEmpty() const;
    bool isFull() const;
    size_t size() const;

    void debugPrint() const;

private:
    uint32_t buffer[Capacity];
    size_t head;
    size_t tail;
    size_t count;
};

#endif // UINT32FIFO_H
