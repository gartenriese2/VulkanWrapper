#include "device.hpp"

namespace bmvk
{
    Device::Device(vk::Device device, const std::uint32_t queueFamilyIndex)
        : m_queueFamilyIndex{queueFamilyIndex}
    {
        m_device = std::move(device);
    }

    std::unique_ptr<Queue> Device::createQueue() const
    {
        return std::make_unique<Queue>(m_device.getQueue(m_queueFamilyIndex, 0));
    }
}
