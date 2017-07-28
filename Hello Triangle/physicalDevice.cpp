#include "physicalDevice.hpp"

namespace bmvk
{
    PhysicalDevice::PhysicalDevice(const vk::PhysicalDevice & physicalDevice, const uint32_t queueFamilyIndex)
        : m_physicalDevice{ physicalDevice },
          m_queueFamilyIndex{ queueFamilyIndex }
    {
    }

    Device PhysicalDevice::createLogicalDevice(const std::vector<const char*> & layerNames, const bool enableValidationLayers) const
    {
        const auto queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo queueCreateInfo(vk::DeviceQueueCreateFlags(), getQueueFamilyIndex(), 1, &queuePriority);
        vk::PhysicalDeviceFeatures deviceFeatures;
        vk::DeviceCreateInfo info(vk::DeviceCreateFlags(), 1, &queueCreateInfo, static_cast<uint32_t>(layerNames.size()), layerNames.data(), 0, nullptr, &deviceFeatures);
        return Device(m_physicalDevice.createDevice(info), m_queueFamilyIndex);
    }

    std::tuple<bool, int> PhysicalDevice::isDeviceSuitable(const vk::PhysicalDevice & device, const vk::SurfaceKHR & surface)
    {
        const auto features = device.getFeatures();
        const auto properties = device.getProperties();
        const auto queueFamilyProperties = device.getQueueFamilyProperties();

        auto index = 0;
        auto foundQueue = false;
        for (const auto& queueFamilyProperty : queueFamilyProperties) {
            if (queueFamilyProperty.queueCount > 0 && queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                foundQueue = true;
                break;
            }

            ++index;
        }

        const auto hasPresentationSupport{ device.getSurfaceSupportKHR(index, surface) };
        return std::make_tuple(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && foundQueue && hasPresentationSupport, index);
    }
}
