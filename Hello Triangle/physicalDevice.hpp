#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"

namespace bmvk
{
    class PhysicalDevice
    {
    public:
        explicit PhysicalDevice(const vk::PhysicalDevice & physicalDevice, const uint32_t queueFamilyIndex);
        PhysicalDevice(const PhysicalDevice &) = delete;
        PhysicalDevice(PhysicalDevice && other) = default;
        PhysicalDevice & operator=(const PhysicalDevice &) = delete;
        PhysicalDevice & operator=(PhysicalDevice && other) = default;
        ~PhysicalDevice() {}

        explicit operator vk::PhysicalDevice() const noexcept { return m_physicalDevice; }

        auto getCPhysicalDevice() noexcept { return static_cast<VkPhysicalDevice>(m_physicalDevice); }
        auto getQueueFamilyIndex() const { return m_queueFamilyIndex; }

        vk::SurfaceCapabilitiesKHR getSurfaceCapabilities(const vk::SurfaceKHR & surface) const { return m_physicalDevice.getSurfaceCapabilitiesKHR(surface); }
        std::vector<vk::SurfaceFormatKHR> getSurfaceFormats(const vk::SurfaceKHR & surface) const { return m_physicalDevice.getSurfaceFormatsKHR(surface); }
        std::vector<vk::PresentModeKHR> getPresentModes(const vk::SurfaceKHR & surface) const { return m_physicalDevice.getSurfacePresentModesKHR(surface); }
        Device createLogicalDevice(const std::vector<const char*> & layerNames) const;
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
        vk::Format findSupportedFormat(const std::vector<vk::Format> & candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;
        vk::Format findDepthFormat() const;

        static std::tuple<bool, int> isDeviceSuitable(const vk::PhysicalDevice & device, const vk::SurfaceKHR & surface);

        static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> & availableFormats);
        static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> & availablePresentModes);
        static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR & capabilities, const uint32_t width, const uint32_t height);
        static uint32_t chooseImageCount(const vk::SurfaceCapabilitiesKHR & capabilities);
    private:
        vk::PhysicalDevice m_physicalDevice;
        uint32_t m_queueFamilyIndex;

        friend class Instance;
        PhysicalDevice() {}

        static bool checkDeviceExtensionSupport(const vk::PhysicalDevice & device);
        static bool checkSwapChainSupport(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
    };

    static_assert(std::is_nothrow_move_constructible_v<PhysicalDevice>);
    static_assert(!std::is_copy_constructible_v<PhysicalDevice>);
    static_assert(std::is_nothrow_move_assignable_v<PhysicalDevice>);
    static_assert(!std::is_copy_assignable_v<PhysicalDevice>);
}

