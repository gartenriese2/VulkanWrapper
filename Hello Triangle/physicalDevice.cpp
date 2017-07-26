#include "physicalDevice.hpp"

namespace bmvk
{
    PhysicalDevice::PhysicalDevice(const std::unique_ptr<Instance> & instancePtr)
    {
        const auto physicalDevices = instancePtr->getInstance().enumeratePhysicalDevices();
        auto foundSuitablePhysicalDevice{ false };
        for (auto physicalDevice : physicalDevices)
        {
            bool isSuitable;
            int queueFamilyIndex;
            std::tie(isSuitable, queueFamilyIndex) = isDeviceSuitable(physicalDevice);
            if (isSuitable)
            {
                foundSuitablePhysicalDevice = true;
                m_physicalDevice = physicalDevice;
                m_queueFamilyIndex = queueFamilyIndex;
                break;
            }
        }

        if (!foundSuitablePhysicalDevice)
        {
            throw std::runtime_error("No suitable physical device found");
        }
    }

    std::unique_ptr<Device> PhysicalDevice::createLogicalDevice(const std::unique_ptr<Instance> & instancePtr, const bool enableValidationLayers) const
    {
        const auto queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo queueCreateInfo(vk::DeviceQueueCreateFlags(), getQueueFamilyIndex(), 1, &queuePriority);
        vk::PhysicalDeviceFeatures deviceFeatures;
        const auto layers = instancePtr->getLayerNames();
        vk::DeviceCreateInfo info(vk::DeviceCreateFlags(), 1, &queueCreateInfo, static_cast<uint32_t>(layers.size()), layers.data(), 0, nullptr, &deviceFeatures);
        return std::make_unique<Device>(m_physicalDevice.createDevice(info), m_queueFamilyIndex);
    }

    std::tuple<bool, int> PhysicalDevice::isDeviceSuitable(const vk::PhysicalDevice & device) const
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

        return std::make_tuple(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && foundQueue, index);
    }
}
