#include "physicalDevice.hpp"
#include <set>

namespace bmvk
{
    constexpr auto k_swapchainExtensionName{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
        std::vector<const char *> extensionNames{ k_swapchainExtensionName };
        vk::DeviceCreateInfo info(vk::DeviceCreateFlags(), 1, &queueCreateInfo, static_cast<uint32_t>(layerNames.size()), layerNames.data(), static_cast<uint32_t>(extensionNames.size()), extensionNames.data(), &deviceFeatures);
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
        const auto hasRequiredExtensions{ checkDeviceExtensionSupport(device) };
        return std::make_tuple(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && foundQueue && hasPresentationSupport && hasRequiredExtensions, index);
    }

    bool PhysicalDevice::checkDeviceExtensionSupport(const vk::PhysicalDevice & device)
    {
        const auto availableExtensions = device.enumerateDeviceExtensionProperties();
        return !(std::find_if(availableExtensions.cbegin(), availableExtensions.cend(), [&](const auto & ex) { return std::strcmp(ex.extensionName, k_swapchainExtensionName) == 0; }) == availableExtensions.cend());
    }
}
