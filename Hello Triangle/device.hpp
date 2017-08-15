#pragma once

#include "vulkan_bmvk.hpp"
#include "queue.hpp"
#include "commandbuffer.hpp"

namespace bmvk
{
    class Swapchain;
    class Shader;

    class Device
    {
    public:
        explicit Device(vk::Device && device, const uint32_t queueFamilyIndex);
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
        void * mapMemory(const vk::UniqueDeviceMemory & memory, const vk::DeviceSize size, const vk::DeviceSize offset = 0, const vk::MemoryMapFlags flags = {}) const;
        void unmapMemory(const vk::UniqueDeviceMemory & memory) const;
        template <class T>
        void copyToMemory(const vk::UniqueDeviceMemory & memory, T & obj) const;
        uint32_t acquireNextImage(const Swapchain & swapchain, OptRefSemaphore semaphore = {}, OptRefFence fence = {}) const;
    private:
        vk::Device m_device;
        uint32_t m_queueFamilyIndex;
    };

    template<class T>
    void Device::copyToMemory(const vk::UniqueDeviceMemory & memory, T & obj) const
    {
        auto data{ mapMemory(memory, sizeof obj) };
        memcpy(data, &obj, sizeof obj);
        unmapMemory(memory);
    }

    static_assert(std::is_move_constructible_v<Device>);
    static_assert(!std::is_copy_constructible_v<Device>);
    static_assert(std::is_move_assignable_v<Device>);
    static_assert(!std::is_copy_assignable_v<Device>);
}
