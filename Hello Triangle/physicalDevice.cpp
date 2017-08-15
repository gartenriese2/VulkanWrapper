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

    Device PhysicalDevice::createLogicalDevice(const std::vector<const char*> & layerNames) const
    {
        auto queuePriority = 1.0f;
        DeviceQueueCreateInfo queueCreateInfo{ {}, getQueueFamilyIndex(), static_cast<uint32_t>(1), vk::ArrayProxy<float>(queuePriority) };
        vk::DeviceQueueCreateInfo vk_queueCreateInfo{ queueCreateInfo };
        vk::PhysicalDeviceFeatures deviceFeatures;
        std::vector<const char *> extensionNames{ k_swapchainExtensionName };
        DeviceCreateInfo info( {}, vk_queueCreateInfo, layerNames, extensionNames, deviceFeatures );
        return Device(std::move(m_physicalDevice.createDevice(info)), m_queueFamilyIndex);
    }

    uint32_t PhysicalDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        const auto memProperties{ m_physicalDevice.getMemoryProperties() };
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
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
        const auto hasSwapChainSupport{ checkSwapChainSupport(device, surface) };
        return std::make_tuple(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && foundQueue && hasPresentationSupport && hasRequiredExtensions && hasSwapChainSupport, index);
    }

    bool PhysicalDevice::checkDeviceExtensionSupport(const vk::PhysicalDevice & device)
    {
        const auto availableExtensions = device.enumerateDeviceExtensionProperties();
        return !(std::find_if(availableExtensions.cbegin(), availableExtensions.cend(), [&](const auto & ex) { return std::strcmp(ex.extensionName, k_swapchainExtensionName) == 0; }) == availableExtensions.cend());
    }

    bool PhysicalDevice::checkSwapChainSupport(const vk::PhysicalDevice & device, const vk::SurfaceKHR & surface)
    {
        const auto capabilities{ device.getSurfaceCapabilitiesKHR(surface) };
        const auto formats{ device.getSurfaceFormatsKHR(surface) };
        const auto presentModes{ device.getSurfacePresentModesKHR(surface) };
        return !formats.empty() && !presentModes.empty();
    }

    vk::SurfaceFormatKHR PhysicalDevice::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> & availableFormats)
    {
        if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
        {
            return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
        }

        for (const auto & availableFormat : availableFormats)
        {
            if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR PhysicalDevice::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> & availablePresentModes)
    {
        for (const auto & availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox)
            {
                return availablePresentMode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D PhysicalDevice::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR & capabilities, const uint32_t width, const uint32_t height)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        vk::Extent2D actualExtent{ width,height };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }

    uint32_t PhysicalDevice::chooseImageCount(const vk::SurfaceCapabilitiesKHR & capabilities)
    {
        auto imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }

        return imageCount;
    }
}
