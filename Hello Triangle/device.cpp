#include "device.hpp"

#include "shader.hpp"

namespace bmvk
{
    Device::Device(vk::Device device, const uint32_t queueFamilyIndex)
      : m_device{ device },
        m_queueFamilyIndex{ queueFamilyIndex }
    {
    }

    Device::~Device()
    {
        m_device.destroy();
    }

    Queue Device::createQueue() const
    {
        return Queue(m_device.getQueue(m_queueFamilyIndex, 0));
    }

    vk::UniqueImageView Device::createImageView(vk::ImageViewCreateInfo info) const
    {
        return m_device.createImageViewUnique(info);
    }

    vk::UniqueCommandPool Device::createCommandPool() const
    {
        vk::CommandPoolCreateInfo poolInfo{ {}, m_queueFamilyIndex };
        return m_device.createCommandPoolUnique(poolInfo);
    }

    CommandBuffer Device::allocateCommandBuffer(const vk::UniqueCommandPool & pool, vk::CommandBufferLevel level) const
    {
        auto vec = m_device.allocateCommandBuffersUnique({ *pool, level, 1 });
        assert(vec.size() == 1);

        return CommandBuffer(std::move(vec[0]));
    }

    std::vector<CommandBuffer> Device::allocateCommandBuffers(const vk::UniqueCommandPool & pool, const uint32_t count, const vk::CommandBufferLevel level) const
    {
        auto vec = m_device.allocateCommandBuffersUnique({ *pool, level, count });
        assert(vec.size() == count);

        std::vector<CommandBuffer> ret;
        for (auto & buffer : vec)
        {
            ret.emplace_back(std::move(buffer));
        }

        return ret;
    }

    vk::UniqueShaderModule Device::createShaderModule(const std::vector<char> & code) const
    {
        vk::ShaderModuleCreateInfo info{ {}, code.size(), reinterpret_cast<const uint32_t *>(code.data()) };
        return m_device.createShaderModuleUnique(info);
    }

    vk::UniqueSemaphore Device::createSemaphore() const
    {
        return m_device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
    }
}
