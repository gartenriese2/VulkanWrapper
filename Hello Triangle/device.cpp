#include "device.hpp"

namespace bmvk
{
    Device::Device(vk::Device device, const std::uint32_t queueFamilyIndex)
      : m_device{ device },
        m_queueFamilyIndex{ queueFamilyIndex }
    {
    }

    Device::~Device()
    {
        if (m_swapchainCreated)
        {
            m_device.destroySwapchainKHR(m_swapchain);
        }
    }

    Queue Device::createQueue() const
    {
        return Queue(m_device.getQueue(m_queueFamilyIndex, 0));
    }

    void Device::createSwapchain(const vk::SurfaceKHR & surface, const uint32_t imageCount, const vk::SurfaceFormatKHR & surfaceFormat, const vk::Extent2D & extent, const vk::SurfaceCapabilitiesKHR & capabilities, const vk::PresentModeKHR & presentMode)
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

        m_swapchain = m_device.createSwapchainKHR(info);
        m_swapchainCreated = true;
    }

    std::vector<vk::Image> Device::getSwapchainImages() const
    {
        return m_swapchainCreated ? m_device.getSwapchainImagesKHR(m_swapchain) : std::vector<vk::Image>();
    }

    vk::UniqueImageView Device::createImageView(vk::ImageViewCreateInfo info) const
    {
        return m_device.createImageViewUnique(info);
    }
}
