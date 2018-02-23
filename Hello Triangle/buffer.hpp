#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class CommandBuffer;

    class Buffer
    {
    public:
        Buffer(const vk::UniqueDevice & device, const vk::DeviceSize size, const vk::BufferUsageFlags usage, const vk::SharingMode sharingMode = vk::SharingMode::eExclusive);
        Buffer(const Buffer &) = delete;
        Buffer(Buffer && other) = default;
        Buffer & operator=(const Buffer &) = delete;
        Buffer & operator=(Buffer &&) & = default;
        ~Buffer() {}

        vk::MemoryRequirements getMemoryRequirements(const vk::UniqueDevice & device) const;

        void bindToMemory(const vk::UniqueDevice & device, const vk::UniqueDeviceMemory & memory, const vk::DeviceSize offset = 0) const;
        void copyToImage(CommandBuffer & cmdBuffer, vk::UniqueImage & image, uint32_t width, uint32_t height) const;
        void copyToBuffer(CommandBuffer & cmdBuffer, vk::UniqueBuffer & buffer, vk::DeviceSize size, vk::DeviceSize srcOffset = 0, vk::DeviceSize dstOffset = 0) const;
    private:
        vk::UniqueBuffer m_buffer;
    };

    static_assert(std::is_move_constructible_v<Buffer>);
    static_assert(!std::is_copy_constructible_v<Buffer>);
    static_assert(std::is_move_assignable_v<Buffer>);
    static_assert(!std::is_copy_assignable_v<Buffer>);
}
