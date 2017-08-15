#include "swapchain.hpp"

#include "physicalDevice.hpp"
#include "surface.hpp"
#include "window.hpp"

namespace bmvk
{
    Swapchain::Swapchain(const PhysicalDevice & physicalDevice, const Surface & surface, const Window & window, const Device & device)
    {
        create(physicalDevice, surface, window, device);
    }

    void Swapchain::recreate(const PhysicalDevice & physicalDevice, const Surface & surface, const Window & window, const Device & device)
    {
        for (auto & imageView : m_imageViews)
        {
            imageView.reset(nullptr);
        }

        m_swapchain.reset(nullptr);

        create(physicalDevice, surface, window, device);
    }

    void Swapchain::create(const PhysicalDevice & physicalDevice, const Surface & surface, const Window & window, const Device & device)
    {
        m_capabilities = physicalDevice.getSurfaceCapabilities(static_cast<vk::SurfaceKHR>(surface));
        m_imageCount = PhysicalDevice::chooseImageCount(m_capabilities);

        const auto formats{ physicalDevice.getSurfaceFormats(static_cast<vk::SurfaceKHR>(surface)) };
        const auto presentModes{ physicalDevice.getPresentModes(static_cast<vk::SurfaceKHR>(surface)) };

        m_imageFormat = PhysicalDevice::chooseSwapSurfaceFormat(formats);
        m_presentMode = PhysicalDevice::chooseSwapPresentMode(presentModes);

        createExtent(window);

        vk::SwapchainCreateInfoKHR createInfo
        {
            vk::SwapchainCreateFlagsKHR(),
            static_cast<vk::SurfaceKHR>(surface),
            m_imageCount,
            m_imageFormat.format,
            m_imageFormat.colorSpace,
            m_extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive,
            0,
            nullptr,
            m_capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            m_presentMode,
            true,
            nullptr
        };

        m_swapchain = static_cast<vk::Device>(device).createSwapchainKHRUnique(createInfo);
        m_images = static_cast<vk::Device>(device).getSwapchainImagesKHR(*m_swapchain);
        m_imageViews.resize(m_images.size());
        for (size_t i = 0; i < m_images.size(); i++)
        {
            const auto info = vk::ImageViewCreateInfo{ {}, m_images[i], vk::ImageViewType::e2D, m_imageFormat.format, {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1) };
            m_imageViews[i] = device.createImageView(info);
        }
    }

    void Swapchain::createExtent(const Window & window)
    {
        int width, height;
        std::tie(width, height) = window.getSize();
        m_extent = PhysicalDevice::chooseSwapExtent(m_capabilities, width, height);
    }
}
