#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "device.hpp"

namespace bmvk
{
    class Device;

    struct StagingBuffer
    {
        StagingBuffer(Device & device, Buffer && _buffer, vk::UniqueDeviceMemory && _memory)
            : buffer{std::move(_buffer)},
              memory{std::move(_memory)},
              m_device{device}
        {
        }

        void fill(const void * const objPtr, size_t objSize, vk::DeviceSize offset = 0) const
        {
            auto data{ reinterpret_cast<vk::DeviceSize *>(m_device.mapMemory(memory, objSize, offset)) };
            memcpy(data + offset, objPtr, objSize);
            m_device.unmapMemory(memory);
        }

        Buffer buffer;
        vk::UniqueDeviceMemory memory;
    private:
        Device & m_device;
    };

    class BufferFactory
    {
    public:
        BufferFactory(Device & device, const vk::PhysicalDevice & physicalDevice);
        BufferFactory(const BufferFactory &) = delete;
        BufferFactory(BufferFactory && other) = default;
        BufferFactory & operator=(const BufferFactory &) = delete;
        BufferFactory & operator=(BufferFactory &&) = delete;

        StagingBuffer createStagingBuffer(const vk::DeviceSize size) const;
    private:
        Device & m_device;
        vk::PhysicalDeviceMemoryProperties m_memoryProperties;

        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
    };

    static_assert(std::is_move_constructible_v<BufferFactory>);
    static_assert(!std::is_copy_constructible_v<BufferFactory>);
    static_assert(!std::is_move_assignable_v<BufferFactory>);
    static_assert(!std::is_copy_assignable_v<BufferFactory>);
}
