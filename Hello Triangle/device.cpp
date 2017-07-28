#include "device.hpp"

namespace bmvk
{
    Device::Device(vk::Device device, const std::uint32_t queueFamilyIndex)
      : m_device{ device },
        m_queueFamilyIndex{ queueFamilyIndex }
    {
    }

    Queue Device::createQueue() const
    {
        return Queue(m_device.getQueue(m_queueFamilyIndex, 0));
    }
}
