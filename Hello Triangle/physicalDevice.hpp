#pragma once

#include <vulkan/vulkan.hpp>
#include "instance.hpp"

namespace bmvk
{
    class PhysicalDevice
    {
    public:
        explicit PhysicalDevice(const std::unique_ptr<Instance> & instancePtr);
        PhysicalDevice(const PhysicalDevice &) = delete;
        PhysicalDevice(PhysicalDevice && other) = default;
        PhysicalDevice & operator=(const PhysicalDevice &) = delete;
        PhysicalDevice & operator=(PhysicalDevice && other) = default;
        ~PhysicalDevice() {}

        auto getCPhysicalDevice() noexcept { return static_cast<VkPhysicalDevice>(m_physicalDevice); }
        auto getQueueFamilyIndex() const { return m_queueFamilyIndex; }

        vk::Device createDevice(const vk::DeviceCreateInfo & info);
    private:
        vk::PhysicalDevice m_physicalDevice;
        uint32_t m_queueFamilyIndex;

        std::tuple<bool, int> isDeviceSuitable(const vk::PhysicalDevice & device) const;
    };

    static_assert(std::is_nothrow_move_constructible_v<PhysicalDevice>);
    static_assert(!std::is_copy_constructible_v<PhysicalDevice>);
    static_assert(std::is_nothrow_move_assignable_v<PhysicalDevice>);
    static_assert(!std::is_copy_assignable_v<PhysicalDevice>);
}

