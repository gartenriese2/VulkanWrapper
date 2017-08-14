#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class CommandBuffer
    {
    public:
        explicit CommandBuffer(vk::UniqueCommandBuffer && commandBuffer) : m_commandBuffer{std::move(commandBuffer)} {}
        CommandBuffer(const CommandBuffer &) = delete;
        CommandBuffer(CommandBuffer && other) = default;
        CommandBuffer & operator=(const CommandBuffer &) = delete;
        CommandBuffer & operator=(CommandBuffer && other) = default;
        ~CommandBuffer() {}

        explicit operator const vk::UniqueCommandBuffer &() const noexcept { return m_commandBuffer; }

        void begin(vk::CommandBufferUsageFlags flags = {}) const;
        void end() const;
        void copyBuffer(vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const;
    private:
        vk::UniqueCommandBuffer m_commandBuffer;
    };

    static_assert(std::is_move_constructible_v<CommandBuffer>);
    static_assert(std::is_move_assignable_v<CommandBuffer>);
    static_assert(!std::is_copy_constructible_v<CommandBuffer>);
    static_assert(!std::is_copy_assignable_v<CommandBuffer>);
}
