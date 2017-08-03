#pragma once
#include <vulkan/vulkan.hpp>
#include "queue.hpp"

namespace bmvk
{
    class Device
    {
    public:
        explicit Device(vk::Device device, const std::uint32_t queueFamilyIndex);
        Device(const Device &) = delete;
        Device(Device && other) = default;
        Device & operator=(const Device &) = delete;
        Device & operator=(Device && other) = default;
        ~Device();

        explicit operator vk::Device() const noexcept { return m_device; }
        const auto & getSwapchain() const noexcept { return m_swapchain; }

        Queue createQueue() const;
        void createSwapchain(const vk::SurfaceKHR & surface, const uint32_t imageCount, const vk::SurfaceFormatKHR & surfaceFormat, const vk::Extent2D & extent, const vk::SurfaceCapabilitiesKHR & capabilities, const vk::PresentModeKHR & presentMode);
        std::vector<vk::Image> getSwapchainImages() const;
        vk::UniqueImageView createImageView(vk::ImageViewCreateInfo info) const;
    private:
        vk::Device m_device;
        uint32_t m_queueFamilyIndex;
        vk::SwapchainKHR m_swapchain;
        bool m_swapchainCreated = false;
    };

    static_assert(std::is_move_constructible_v<Device>);
    static_assert(!std::is_copy_constructible_v<Device>);
    static_assert(std::is_move_assignable_v<Device>);
    static_assert(!std::is_copy_assignable_v<Device>);
}
