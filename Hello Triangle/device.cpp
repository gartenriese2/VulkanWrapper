#include "device.hpp"

namespace bmvk
{
    Device::Device(vk::Device device)
    {
        m_device = std::move(device);
    }
}