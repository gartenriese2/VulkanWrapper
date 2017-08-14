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
}