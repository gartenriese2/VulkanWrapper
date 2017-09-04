#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class CommandBuffer;

    class Buffer
    {
    public:
        Buffer(const vk::Device & device, const vk::DeviceSize size, const vk::BufferUsageFlags usage);
        Buffer(const Buffer &) = delete;
        Buffer(Buffer && other) = default;
        Buffer & operator=(const Buffer &) = delete;
        Buffer & operator=(Buffer &&) & = default;
        ~Buffer() {}

        vk::MemoryRequirements getMemoryRequirements(const vk::Device & device) const;

        void bindToMemory(const vk::Device & device, const vk::UniqueDeviceMemory & memory, const vk::DeviceSize offset = 0) const;
        void copyToImage(CommandBuffer & cmdBuffer, vk::UniqueImage & image, uint32_t width, uint32_t height) const;
    private:
        vk::UniqueBuffer m_buffer;
    };

    static_assert(std::is_move_constructible_v<Buffer>);
    static_assert(!std::is_copy_constructible_v<Buffer>);
    static_assert(std::is_move_assignable_v<Buffer>);
    static_assert(!std::is_copy_assignable_v<Buffer>);
}
