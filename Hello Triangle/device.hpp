#pragma once

#include <vulkan/vulkan.hpp>

#include "queue.hpp"
#include "commandbuffer.hpp"

namespace bmvk
{
    class Shader;

    class Device
    {
    public:
        explicit Device(vk::Device device, const uint32_t queueFamilyIndex);
        Device(const Device &) = delete;
        Device(Device && other) = default;
        Device & operator=(const Device &) = delete;
        Device & operator=(Device && other) = default;
        ~Device();

        explicit operator const vk::Device &() const noexcept { return m_device; }

        Queue createQueue() const;
        vk::UniqueImageView createImageView(vk::ImageViewCreateInfo info) const;
        vk::UniqueShaderModule createShaderModule(const std::vector<char> & code) const;
        vk::UniqueSemaphore createSemaphore() const;
        vk::UniqueCommandPool createCommandPool() const;
        CommandBuffer allocateCommandBuffer(const vk::UniqueCommandPool & pool, const vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
        std::vector<CommandBuffer> allocateCommandBuffers(const vk::UniqueCommandPool & pool, const uint32_t count, const vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
        void waitIdle() const { m_device.waitIdle(); }
    private:
        vk::Device m_device;
        uint32_t m_queueFamilyIndex;
    };

    static_assert(std::is_move_constructible_v<Device>);
    static_assert(!std::is_copy_constructible_v<Device>);
    static_assert(std::is_move_assignable_v<Device>);
    static_assert(!std::is_copy_assignable_v<Device>);
}
