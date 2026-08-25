#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace meeting_sdk::audio {

// Single-producer, single-consumer, lock-free, fixed-capacity ring buffer. Sized for the
// realtime capture thread -> VAD worker thread handoff: the producer (capture callback) must
// never block on a mutex (see docs/architecture/05-diagrams.md §4), so both operations only
// ever touch atomics.
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity)
        : capacity_(capacity + 1), buffer_(capacity_) {}  // +1 slot distinguishes full from empty

    // Producer-thread only. Returns false if the buffer is full; the caller decides the
    // overload policy (the expected default is to drop the incoming frame).
    bool tryPush(T value) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t nextHead = advance(head);
        if (nextHead == tail_.load(std::memory_order_acquire)) {
            return false;  // full
        }
        buffer_[head] = std::move(value);
        head_.store(nextHead, std::memory_order_release);
        return true;
    }

    // Consumer-thread only.
    std::optional<T> tryPop() {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;  // empty
        }
        T value = std::move(buffer_[tail]);
        tail_.store(advance(tail), std::memory_order_release);
        return value;
    }

    std::size_t capacity() const noexcept { return capacity_ - 1; }

private:
    std::size_t advance(std::size_t index) const noexcept { return (index + 1) % capacity_; }

    std::size_t capacity_;
    std::vector<T> buffer_;
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};

}  // namespace meeting_sdk::audio
