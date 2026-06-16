#pragma once

#include "wtpch.h"

template <typename T>
class RingBuffer {
public:
    // size must be a power of 2 for optimal performance (allows bitwise masking)
    explicit RingBuffer(size_t capacity)
        : m_Capacity(capacity)
        , m_Buffer(capacity)
        , m_WriteIndex(0)
        , m_ReadIndex(0)
    {
        // enforce power-of-two capacity for fast wrapping mask
        m_Mask = m_Capacity - 1;
        // basic check to ensure capacity is indeed a power of two
        if ((m_Capacity & m_Mask) != 0) {
            std::cerr << "[RingBuffer] Capacity must be a power of 2.\n";
        }
    }

    // producer method
    // returns the number of elements successfully written
    size_t Write(const T* data, size_t count) {
        // acquire current indices
        size_t writeIndex = m_WriteIndex.load(std::memory_order_relaxed);
        size_t readIndex = m_ReadIndex.load(std::memory_order_acquire);

        // calculate available space in the buffer
        size_t availableSpace = m_Capacity - (writeIndex - readIndex);
        size_t toWrite = std::min(count, availableSpace);

        if (toWrite == 0) return 0;

        // copy data into the ring buffer
        for (size_t i = 0; i < toWrite; ++i) {
            m_Buffer[(writeIndex + i) & m_Mask] = data[i];
        }

        m_WriteIndex.store(writeIndex + toWrite, std::memory_order_release);
        return toWrite;
    }

    // consumer method
    // returns the number of elements successfully read
    size_t Read(T* destination, size_t count) {
        // acquire current indices
        size_t writeIndex = m_WriteIndex.load(std::memory_order_acquire);
        size_t readIndex = m_ReadIndex.load(std::memory_order_relaxed);

        // calculate available data to be read
        size_t availableData = writeIndex - readIndex;
        size_t toRead = std::min(count, availableData);

        if (toRead == 0) return 0;

        // copy data out of the ring buffer into the destination array
        for (size_t i = 0; i < toRead; ++i) {
            destination[i] = m_Buffer[(readIndex + i) & m_Mask];
        }

        // release the new read index so the producer knows space has freed up
        m_ReadIndex.store(readIndex + toRead, std::memory_order_release);
        return toRead;
    }

    // helper to check how many samples are waiting to be read
    size_t GetAvailableRead() const {
        size_t writeIndex = m_WriteIndex.load(std::memory_order_relaxed);
        size_t readIndex = m_ReadIndex.load(std::memory_order_relaxed);
        return writeIndex - readIndex;
    }

private:
    size_t m_Capacity;
    size_t m_Mask;

    std::vector<T> m_Buffer;

    // use atomic size_t for thread-safe index sharing
    // aligning them avoids "false sharing" performance degradation across CPU cache lines
    alignas(64) std::atomic<size_t> m_WriteIndex;
    alignas(64) std::atomic<size_t> m_ReadIndex;
};