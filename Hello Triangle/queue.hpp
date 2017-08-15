#pragma once

#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class CommandBuffer;

    class Queue
    {
    public:
        explicit Queue(vk::Queue && queue);
        Queue(const Queue &) = delete;
        Queue(Queue && other) = default;
        Queue & operator=(const Queue &) = delete;
        Queue & operator=(Queue &&) & = default;
        ~Queue() {}

        explicit operator const vk::Queue &() const noexcept { return m_queue; }

        void waitIdle() const { m_queue.waitIdle(); }
        void submit(const CommandBuffer & cmdBuffer, vk::Fence fence = nullptr) const;
        void submit(const CommandBuffer & cmdBuffer, const vk::UniqueSemaphore & waitSemaphore, const vk::UniqueSemaphore & signalSemaphore, vk::PipelineStageFlags flags, vk::Fence fence = nullptr) const;
        bool present(vk::ArrayProxy<vk::Semaphore> waitSemaphores = nullptr, vk::ArrayProxy<vk::SwapchainKHR> swapchains = nullptr, vk::ArrayProxy<uint32_t> imageIndices = nullptr, vk::ArrayProxy<vk::Result> results = nullptr) const;
    private:
        vk::Queue m_queue;
    };

    static_assert(std::is_nothrow_move_constructible_v<Queue>);
    static_assert(!std::is_copy_constructible_v<Queue>);
    static_assert(std::is_nothrow_move_assignable_v<Queue>);
    static_assert(!std::is_copy_assignable_v<Queue>);
}
