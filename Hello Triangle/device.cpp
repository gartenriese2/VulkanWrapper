#include "device.hpp"

namespace bmvk
{
    Device::Device(vk::Device device, const std::uint32_t queueFamilyIndex)
        : m_queueFamilyIndex{queueFamilyIndex}
    {
        m_device = std::move(device);
    }

    Queue Device::createQueue() const
    {
        return Queue(m_device.getQueue(m_queueFamilyIndex, 0));
    }
}
