#include "swapchain.hpp"

#include "physicalDevice.hpp"
#include "surface.hpp"

namespace bmvk
{
    Swapchain::Swapchain(const PhysicalDevice & physicalDevice, const Surface & surface, const std::tuple<int32_t, int32_t> & windowSize, const Device & device)
    {
        create(physicalDevice, surface, windowSize, device);
    }

    void Swapchain::recreate(const PhysicalDevice & physicalDevice, const Surface & surface, const std::tuple<int32_t, int32_t> & windowSize, const Device & device)
    {
        for (auto & imageView : m_imageViews)
        {
            imageView.reset(nullptr);
        }

        m_swapchain.reset(nullptr);

        create(physicalDevice, surface, windowSize, device);
    }

    vk::PipelineViewportStateCreateInfo Swapchain::getPipelineViewportStateCreateInfo(vk::Viewport & viewport, vk::Rect2D & scissor, const float minDepth, const float maxDepth) const
    {
        viewport.setX(0.f);
        viewport.setY(0.f);
        viewport.setWidth(static_cast<float>(m_extent.width));
        viewport.setHeight(static_cast<float>(m_extent.height));
        viewport.setMinDepth(minDepth);
        viewport.setMaxDepth(maxDepth);
        scissor.setOffset({});
        scissor.setExtent(m_extent);
        return { {}, 1, &viewport, 1, &scissor };
    }

    void Swapchain::create(const PhysicalDevice & physicalDevice, const Surface & surface, const std::tuple<int32_t, int32_t> & windowSize, const Device & device)
    {
        m_capabilities = physicalDevice.getSurfaceCapabilities(reinterpret_cast<const vk::UniqueSurfaceKHR &>(surface));
        m_imageCount = PhysicalDevice::chooseImageCount(m_capabilities);

        const auto formats{ physicalDevice.getSurfaceFormats(reinterpret_cast<const vk::UniqueSurfaceKHR &>(surface)) };
        const auto presentModes{ physicalDevice.getPresentModes(reinterpret_cast<const vk::UniqueSurfaceKHR &>(surface)) };

        m_imageFormat = PhysicalDevice::chooseSwapSurfaceFormat(formats);
        m_presentMode = PhysicalDevice::chooseSwapPresentMode(presentModes);

        createExtent(windowSize);

        vk::SwapchainCreateInfoKHR createInfo
        {
            vk::SwapchainCreateFlagsKHR(),
            *reinterpret_cast<const vk::UniqueSurfaceKHR &>(surface),
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

        m_swapchain = reinterpret_cast<const vk::UniqueDevice &>(device)->createSwapchainKHRUnique(createInfo);
        m_images = reinterpret_cast<const vk::UniqueDevice &>(device)->getSwapchainImagesKHR(*m_swapchain);
        m_imageViews.resize(m_images.size());
        for (size_t i = 0; i < m_images.size(); i++)
        {
            const auto info = vk::ImageViewCreateInfo{ {}, m_images[i], vk::ImageViewType::e2D, m_imageFormat.format, {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1) };
            m_imageViews[i] = device.createImageView(info);
        }
    }

    void Swapchain::createExtent(const std::tuple<int32_t, int32_t> & windowSize)
    {
        int width, height;
        std::tie(width, height) = windowSize;
        m_extent = PhysicalDevice::chooseSwapExtent(m_capabilities, width, height);
    }
}
