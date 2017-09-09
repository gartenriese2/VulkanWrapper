#pragma once

#include "vulkan_bmvk.hpp"
#include "queue.hpp"
#include "commandbuffer.hpp"
#include "sampler.hpp"

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
        vk::UniqueFramebuffer createFramebuffer(const vk::UniqueRenderPass & renderpass, vk::ArrayProxy<vk::ImageView> attachments = nullptr, uint32_t width = 0, uint32_t height = 0, uint32_t layers = 0) const;
        vk::UniqueShaderModule createShaderModule(const std::vector<char> & code) const;
        vk::UniqueSemaphore createSemaphore() const;
        vk::UniqueCommandPool createCommandPool() const;
        vk::UniqueDescriptorPool createDescriptorPool(vk::DescriptorPoolCreateFlags flags = vk::DescriptorPoolCreateFlags(), uint32_t maxSets = 0, vk::ArrayProxy<vk::DescriptorPoolSize> poolSizes = nullptr) const;
        CommandBuffer allocateCommandBuffer(const vk::UniqueCommandPool & pool, const vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
        std::vector<CommandBuffer> allocateCommandBuffers(const vk::UniqueCommandPool & pool, const uint32_t count, const vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
        Sampler createSampler(const bool enableAnisotropy = false, const float minLod = 0.f, const float maxLod = 0.f) const;
        vk::UniqueDescriptorSetLayout createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> & bindings) const;
        vk::UniquePipelineLayout createPipelineLayout(const std::vector<vk::DescriptorSetLayout> & setLayouts, const std::vector<vk::PushConstantRange> & pushConstantRanges = {}) const;

        void waitIdle() const { m_device.waitIdle(); }
        void * mapMemory(const vk::UniqueDeviceMemory & memory, const vk::DeviceSize size, const vk::DeviceSize offset = 0, const vk::MemoryMapFlags flags = {}) const;
        void unmapMemory(const vk::UniqueDeviceMemory & memory) const;
        template <class T>
        void copyToMemory(const vk::UniqueDeviceMemory & memory, T & obj) const;
        void copyToMemory(const vk::UniqueDeviceMemory & memory, const void * const objPtr, size_t objSize) const;
        uint32_t acquireNextImage(const Swapchain & swapchain, OptRefSemaphore semaphore = {}, OptRefFence fence = {}) const;
        void updateDescriptorSet(vk::WriteDescriptorSet set) const;
        void updateDescriptorSets(vk::ArrayProxy<const vk::WriteDescriptorSet> sets) const;
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
