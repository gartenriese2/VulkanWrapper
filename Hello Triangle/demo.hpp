#pragma once

#include <chrono>
#include <type_traits>
#include <vulkan/vulkan.hpp>

#include "window.hpp"
#include "instance.hpp"
#include "device.hpp"
#include "queue.hpp"

namespace bmvk
{
    class Demo
    {
    public:
        Demo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name, const bool onlyWarningsAndAbove = false);
        Demo(const Demo &) = delete;
        Demo(Demo && other) = default;
        Demo & operator=(const Demo &) = delete;
        Demo & operator=(Demo && other) = default;
        virtual ~Demo() {}

        virtual void run() {}
    protected:
        Window m_window;
        Instance m_instance;
        Device m_device;
        Queue m_queue;
        vk::UniqueCommandPool m_commandPool;

        double m_avgFrameTime = 0.0;
        double m_avgFps = 0.0;

        void copyBuffer(vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const;
        void copyBufferToImage(vk::UniqueBuffer & buffer, vk::UniqueImage & image, uint32_t width, uint32_t height) const;
        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory);
        void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueImage & image, vk::UniqueDeviceMemory & imageMemory);
        void transitionImageLayout(const vk::UniqueImage & image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;
        void timing(const bool print = true);

    private:
        std::chrono::steady_clock::time_point m_timepoint;
        uint32_t m_timepointCount;
        std::chrono::microseconds m_elapsedTime;
    };

    static_assert(std::is_move_constructible_v<Demo>);
    static_assert(std::is_move_assignable_v<Demo>);
    static_assert(!std::is_copy_constructible_v<Demo>);
    static_assert(!std::is_copy_assignable_v<Demo>);
}
