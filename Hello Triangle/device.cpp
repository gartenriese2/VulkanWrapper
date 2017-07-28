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

    vk::SwapchainKHR Device::createSwapchain(const vk::SurfaceKHR & surface, const uint32_t imageCount, const vk::SurfaceFormatKHR & surfaceFormat, const vk::Extent2D & extent, const vk::SurfaceCapabilitiesKHR & capabilities, const vk::PresentModeKHR & presentMode)
    {
        vk::SwapchainCreateInfoKHR info
        {
            vk::SwapchainCreateFlagsKHR(),
            surface,
            imageCount,
            surfaceFormat.format,
            surfaceFormat.colorSpace,
            extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive,
            0,
            nullptr,
            capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            presentMode,
            true,
            nullptr
        };

        return m_device.createSwapchainKHR(info);
    }

    std::vector<vk::Image> Device::getSwapchainImages(const vk::SwapchainKHR & swapchain) const
    {
        return m_device.getSwapchainImagesKHR(swapchain);
    }
}
