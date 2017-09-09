#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class Device;
    class PhysicalDevice;
    class Surface;
    class Window;

    class Swapchain
    {
    public:
        Swapchain(const PhysicalDevice & physicalDevice, const Surface & surface, const Window & window, const Device & device);
        Swapchain(const Swapchain &) = delete;
        Swapchain(Swapchain && other) = default;
        Swapchain & operator=(const Swapchain &) = delete;
        Swapchain & operator=(Swapchain && other) = default;
        ~Swapchain() {}

        explicit operator vk::SwapchainKHR() const noexcept { return *m_swapchain; }

        const auto & getImageFormat() const noexcept { return m_imageFormat; }
        const auto & getExtent() const noexcept { return m_extent; }
        const auto & getImageViews() const noexcept { return m_imageViews; }

        void recreate(const PhysicalDevice & physicalDevice, const Surface & surface, const Window & window, const Device & device);

        vk::PipelineViewportStateCreateInfo getPipelineViewportStateCreateInfo(vk::Viewport & viewport, vk::Rect2D & scissor, const float minDepth = 0.f, const float maxDepth = 1.f) const;
    private:
        vk::UniqueSwapchainKHR m_swapchain;
        vk::SurfaceFormatKHR m_imageFormat;
        vk::Extent2D m_extent;
        std::vector<vk::Image> m_images;
        std::vector<vk::UniqueImageView> m_imageViews;
        vk::SurfaceCapabilitiesKHR m_capabilities;
        uint32_t m_imageCount;
        vk::PresentModeKHR m_presentMode;

        void create(const PhysicalDevice & physicalDevice, const Surface & surface, const Window & window, const Device & device);
        void createExtent(const Window & window);
    };

    static_assert(std::is_move_constructible_v<Swapchain>);
    static_assert(std::is_move_assignable_v<Swapchain>);
    static_assert(!std::is_copy_constructible_v<Swapchain>);
    static_assert(!std::is_copy_assignable_v<Swapchain>);
}
