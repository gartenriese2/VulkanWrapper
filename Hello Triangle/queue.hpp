#pragma once

#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class CommandBuffer;

    class Queue
    {
    public:
        explicit Queue(const vk::Queue & queue);
        Queue(const Queue &) = delete;
        Queue(Queue && other) = default;
        Queue & operator=(const Queue &) = delete;
        Queue & operator=(Queue &&) & = default;
        ~Queue() {}

        explicit operator const vk::Queue &() const noexcept { return m_queue; }

        void waitIdle() const { m_queue.waitIdle(); }
        void submit(const CommandBuffer & cmdBuffer, vk::Fence fence = nullptr) const;
    private:
        vk::Queue m_queue;
    };

    static_assert(std::is_nothrow_move_constructible_v<Queue>);
    static_assert(!std::is_copy_constructible_v<Queue>);
    static_assert(std::is_nothrow_move_assignable_v<Queue>);
    static_assert(!std::is_copy_assignable_v<Queue>);
}
