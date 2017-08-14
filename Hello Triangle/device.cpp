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
