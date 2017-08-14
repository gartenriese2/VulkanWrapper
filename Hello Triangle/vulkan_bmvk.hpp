#pragma once

#include <vulkan/vulkan.hpp>

namespace bmvk
{
    struct CommandBufferAllocateInfo
    {
        explicit CommandBufferAllocateInfo(const vk::UniqueCommandPool & commandPool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary, uint32_t count = 0)
            : m_internal{ commandPool.get(), level, count }
        {
        }

        operator const vk::CommandBufferAllocateInfo &() const noexcept
        {
            return m_internal;
        }

        bool operator==(const CommandBufferAllocateInfo & rhs) const
        {
            return m_internal == rhs.m_internal;
        }

        bool operator!=(const CommandBufferAllocateInfo & rhs) const
        {
            return !operator==(rhs);
        }

    private:
        vk::CommandBufferAllocateInfo m_internal;
    };

    static_assert(sizeof(CommandBufferAllocateInfo) == sizeof(vk::CommandBufferAllocateInfo));
    static_assert(std::is_copy_constructible_v<CommandBufferAllocateInfo>);
    static_assert(std::is_move_constructible_v<CommandBufferAllocateInfo>);
    static_assert(std::is_copy_assignable_v<CommandBufferAllocateInfo>);
    static_assert(std::is_move_assignable_v<CommandBufferAllocateInfo>);

    struct SubmitInfo
    {
        explicit SubmitInfo(vk::ArrayProxy<vk::Semaphore> waitSemaphores, vk::PipelineStageFlags * waitDstStageFlags = nullptr, vk::ArrayProxy<vk::CommandBuffer> commandBuffers = nullptr, vk::ArrayProxy<vk::Semaphore> signalSemaphores = nullptr)
            : m_internal{ waitSemaphores.size(), waitSemaphores.data(), waitDstStageFlags, commandBuffers.size(), commandBuffers.data(), signalSemaphores.size(), signalSemaphores.data() }
        {
        }

        explicit SubmitInfo(const vk::UniqueCommandBuffer & commandBuffer)
            : m_internal{ 0, nullptr, nullptr, 1, &*commandBuffer }
        {
        }

        explicit SubmitInfo(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniqueSemaphore & waitSemaphore, const vk::UniqueSemaphore & signalSemaphore, vk::PipelineStageFlags flags)
            : m_internal{ 1, &*waitSemaphore, &flags, 1, &*commandBuffer, 1, &*signalSemaphore}
        {
        }

        operator const vk::SubmitInfo &() const
        {
            return m_internal;
        }

        bool operator==(SubmitInfo const& rhs) const
        {
            return m_internal == rhs.m_internal;
        }

        bool operator!=(SubmitInfo const& rhs) const
        {
            return !operator==(rhs);
        }

    private:
        vk::SubmitInfo m_internal;
    };

    static_assert(sizeof(SubmitInfo) == sizeof(vk::SubmitInfo));
}