#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"

namespace bmvk
{
    class PhysicalDevice;

    struct StagingBuffer
    {
        StagingBuffer(Buffer && _buffer, vk::UniqueDeviceMemory && _memory)
            : buffer{std::move(_buffer)},
              memory{std::move(_memory)}
        {
        }

        Buffer buffer;
        vk::UniqueDeviceMemory memory;
    };

    class BufferFactory
    {
    public:
        BufferFactory();
        BufferFactory(const BufferFactory &) = delete;
        BufferFactory(BufferFactory && other) = default;
        BufferFactory & operator=(const BufferFactory &) = delete;
        BufferFactory & operator=(BufferFactory &&) & = default;
        ~BufferFactory() {}

        StagingBuffer createStagingBuffer(const vk::Device & device, const PhysicalDevice & physicalDevice, const vk::DeviceSize size) const;
    };

    static_assert(std::is_move_constructible_v<BufferFactory>);
    static_assert(!std::is_copy_constructible_v<BufferFactory>);
    static_assert(std::is_move_assignable_v<BufferFactory>);
    static_assert(!std::is_copy_assignable_v<BufferFactory>);
}
