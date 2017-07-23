#pragma once
#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class Device
    {
    public:
        Device(vk::Device device);
        Device(const Device &) = delete;
        Device(Device && other) = default;
        Device & operator=(const Device &) = delete;
        Device & operator=(Device && other) = default;
        ~Device() {}
    private:
        vk::Device m_device;
        vk::Queue m_queue;
    };

    static_assert(std::is_move_constructible_v<Device>);
    static_assert(!std::is_copy_constructible_v<Device>);
    static_assert(std::is_move_assignable_v<Device>);
    static_assert(!std::is_copy_assignable_v<Device>);
}
