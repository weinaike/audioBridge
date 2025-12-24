#ifndef AUDIOBRIDGE_RINGBUFFER_H
#define AUDIOBRIDGE_RINGBUFFER_H

#include <atomic>
#include <cstring>
#include <algorithm>

namespace audiobridge {

/**
 * Lock-free Single Producer Single Consumer (SPSC) ring buffer
 * Designed for real-time audio applications
 *
 * Features:
 * - Lock-free operations using atomic indices
 * - Power-of-2 size for efficient modulo operation
 * - No dynamic memory allocation in read/write operations
 * - Cache-friendly design
 *
 * @tparam T Element type (typically float for audio)
 */
template<typename T, size_t Size>
class RingBuffer {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");

public:
    static constexpr size_t kCapacity = Size;

    RingBuffer() : readPos_(0), writePos_(0) {
        std::fill(buffer_, buffer_ + Size, T{0});
    }

    ~RingBuffer() = default;

    // Disable copy
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    /**
     * Write data to ring buffer
     * @param data Source data pointer
     * @param count Number of elements to write
     * @return Actual number of elements written
     * @note RT-safe, can be called from audio thread
     */
    size_t Write(const T* data, size_t count) {
        const size_t writePos = writePos_.load(std::memory_order_relaxed);
        const size_t readPos = readPos_.load(std::memory_order_acquire);

        const size_t available = AvailableForWrite(readPos, writePos);
        const size_t toWrite = std::min(count, available);

        if (toWrite == 0) {
            return 0;
        }

        const size_t writeIndex = writePos & (Size - 1);
        const size_t contiguous = Size - writeIndex;

        if (toWrite <= contiguous) {
            // Single contiguous write
            std::memcpy(&buffer_[writeIndex], data, toWrite * sizeof(T));
        } else {
            // Wrap-around write
            std::memcpy(&buffer_[writeIndex], data, contiguous * sizeof(T));
            std::memcpy(buffer_, data + contiguous, (toWrite - contiguous) * sizeof(T));
        }

        writePos_.store(writePos + toWrite, std::memory_order_release);
        return toWrite;
    }

    /**
     * Read data from ring buffer
     * @param data Destination data pointer
     * @param count Number of elements to read
     * @return Actual number of elements read
     * @note RT-safe, can be called from audio thread
     */
    size_t Read(T* data, size_t count) {
        const size_t readPos = readPos_.load(std::memory_order_relaxed);
        const size_t writePos = writePos_.load(std::memory_order_acquire);

        const size_t available = AvailableForRead(readPos, writePos);
        const size_t toRead = std::min(count, available);

        if (toRead == 0) {
            return 0;
        }

        const size_t readIndex = readPos & (Size - 1);
        const size_t contiguous = Size - readIndex;

        if (toRead <= contiguous) {
            // Single contiguous read
            std::memcpy(data, &buffer_[readIndex], toRead * sizeof(T));
        } else {
            // Wrap-around read
            std::memcpy(data, &buffer_[readIndex], contiguous * sizeof(T));
            std::memcpy(data + contiguous, buffer_, (toRead - contiguous) * sizeof(T));
        }

        readPos_.store(readPos + toRead, std::memory_order_release);
        return toRead;
    }

    /**
     * Get available elements for reading
     * @note RT-safe
     */
    size_t AvailableForRead() const {
        const size_t readPos = readPos_.load(std::memory_order_relaxed);
        const size_t writePos = writePos_.load(std::memory_order_acquire);
        return AvailableForRead(readPos, writePos);
    }

    /**
     * Get available space for writing
     * @note RT-safe
     */
    size_t AvailableForWrite() const {
        const size_t writePos = writePos_.load(std::memory_order_relaxed);
        const size_t readPos = readPos_.load(std::memory_order_acquire);
        return AvailableForWrite(readPos, writePos);
    }

    /**
     * Reset buffer to empty state
     * @note NOT thread-safe, use when buffer is not being accessed
     */
    void Reset() {
        readPos_.store(0, std::memory_order_relaxed);
        writePos_.store(0, std::memory_order_relaxed);
    }

private:
    static size_t AvailableForRead(size_t readPos, size_t writePos) {
        return writePos - readPos;
    }

    static size_t AvailableForWrite(size_t readPos, size_t writePos) {
        return Size - (writePos - readPos);
    }

    T buffer_[Size];
    std::atomic<size_t> readPos_;   // Read position
    std::atomic<size_t> writePos_;  // Write position
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_RINGBUFFER_H
