#include "device.hpp"

#include "shader.hpp"
#include "swapchain.hpp"

namespace bmvk
{
    Device::Device(vk::UniqueDevice && device, const uint32_t queueFamilyIndex)
      : m_device{ std::move(device) },
        m_queueFamilyIndex{ queueFamilyIndex }
    {
    }

    Queue Device::createQueue() const
    {
        return Queue(m_device->getQueue(m_queueFamilyIndex, 0));
    }

    vk::UniqueImageView Device::createImageView(vk::ImageViewCreateInfo info) const
    {
        return m_device->createImageViewUnique(info);
    }

    vk::UniqueFramebuffer Device::createFramebuffer(const vk::UniqueRenderPass & renderpass, vk::ArrayProxy<vk::ImageView> attachments, uint32_t width, uint32_t height, uint32_t layers) const
    {
        vk::FramebufferCreateInfo info{ {}, *renderpass, attachments.size(), attachments.data(), width, height, layers };
        return m_device->createFramebufferUnique(info);
    }

    vk::UniqueShaderModule Device::createShaderModule(const std::vector<char> & code) const
    {
        ShaderModuleCreateInfo info{ code };
        return m_device->createShaderModuleUnique(info);
    }

    vk::UniqueSemaphore Device::createSemaphore() const
    {
        return m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
    }

    vk::UniqueCommandPool Device::createCommandPool() const
    {
        vk::CommandPoolCreateInfo poolInfo{ {}, m_queueFamilyIndex };
        return m_device->createCommandPoolUnique(poolInfo);
    }

    vk::UniqueDescriptorPool Device::createDescriptorPool(vk::DescriptorPoolCreateFlags flags, uint32_t maxSets, vk::ArrayProxy<vk::DescriptorPoolSize> poolSizes) const
    {
        DescriptorPoolCreateInfo poolInfo{ flags, maxSets, poolSizes };
        return m_device->createDescriptorPoolUnique(poolInfo);
    }

    CommandBuffer Device::allocateCommandBuffer(const vk::UniqueCommandPool & pool, vk::CommandBufferLevel level) const
    {
        auto vec = m_device->allocateCommandBuffersUnique({ *pool, level, 1 });
        assert(vec.size() == 1);

        return CommandBuffer(std::move(vec[0]));
    }

    std::vector<CommandBuffer> Device::allocateCommandBuffers(const vk::UniqueCommandPool & pool, const uint32_t count, const vk::CommandBufferLevel level) const
    {
        auto vec = m_device->allocateCommandBuffersUnique({ *pool, level, count });
        assert(vec.size() == count);

        std::vector<CommandBuffer> ret;
        for (auto & buffer : vec)
        {
            ret.emplace_back(std::move(buffer));
        }

        return ret;
    }

    Sampler Device::createSampler(const bool enableAnisotropy, const float minLod, const float maxLod) const
    {
        return Sampler(m_device, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.f, enableAnisotropy, enableAnisotropy ? 16.f : 1.f, false, vk::CompareOp::eAlways, minLod, maxLod, vk::BorderColor::eIntOpaqueBlack, false);
    }

    vk::UniqueDescriptorSetLayout Device::createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> & bindings) const
    {
        DescriptorSetLayoutCreateInfo info{ bindings };
        return m_device->createDescriptorSetLayoutUnique(info);
    }

    vk::UniquePipelineLayout Device::createPipelineLayout(const std::vector<vk::DescriptorSetLayout> & setLayouts, const std::vector<vk::PushConstantRange> & pushConstantRanges) const
    {
        PipelineLayoutCreateInfo info{ setLayouts, pushConstantRanges };
        return m_device->createPipelineLayoutUnique(info);
    }

    void * Device::mapMemory(const vk::UniqueDeviceMemory & memory, const vk::DeviceSize size, const vk::DeviceSize offset, const vk::MemoryMapFlags flags) const
    {
        return m_device->mapMemory(*memory, offset, size, flags);
    }

    void Device::unmapMemory(const vk::UniqueDeviceMemory & memory) const
    {
        m_device->unmapMemory(*memory);
    }

    void Device::copyToMemory(const vk::UniqueDeviceMemory & memory, const void * const objPtr, size_t objSize) const
    {
        auto data{ mapMemory(memory, objSize) };
        memcpy(data, objPtr, objSize);
        unmapMemory(memory);
    }

    uint32_t Device::acquireNextImage(const Swapchain & swapchain, OptRefSemaphore semaphore, OptRefFence fence) const
    {
        return m_device->acquireNextImageKHR(*reinterpret_cast<const vk::UniqueSwapchainKHR &>(swapchain), std::numeric_limits<uint64_t>::max(), semaphore, fence).value;
    }

    void Device::updateDescriptorSet(vk::WriteDescriptorSet set) const
    {
        m_device->updateDescriptorSets(set, nullptr);
    }

    void Device::updateDescriptorSets(vk::ArrayProxy<const vk::WriteDescriptorSet> sets) const
    {
        m_device->updateDescriptorSets(sets, nullptr);
    }
}
