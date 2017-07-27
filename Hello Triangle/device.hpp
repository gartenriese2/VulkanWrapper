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
        ~Device() {}

        Queue createQueue() const;
    private:
        vk::Device m_device;
        uint32_t m_queueFamilyIndex;
    };

    static_assert(std::is_move_constructible_v<Device>);
    static_assert(!std::is_copy_constructible_v<Device>);
    static_assert(std::is_move_assignable_v<Device>);
    static_assert(!std::is_copy_assignable_v<Device>);
}
