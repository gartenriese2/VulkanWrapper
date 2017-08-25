#include "demo.hpp"

#include <iostream>

namespace bmvk
{
    Demo::Demo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name, const bool onlyWarningsAndAbove)
      : m_window{width, height, name},
        m_instance{name, VK_MAKE_VERSION(1, 0, 0), "bmvk", VK_MAKE_VERSION(1, 0, 0), m_window, enableValidationLayers, onlyWarningsAndAbove},
        m_device{m_instance.getPhysicalDevice().createLogicalDevice(m_instance.getLayerNames())},
        m_queue{m_device.createQueue()},
        m_commandPool{m_device.createCommandPool()},
        m_timepoint{std::chrono::steady_clock::now()},
        m_timepointCount{0},
        m_elapsedTime{std::chrono::microseconds::zero()}
    {
    }

    void Demo::copyBuffer(vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const
    {
        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmdBuffer.copyBuffer(srcBuffer, dstBuffer, size);
        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_queue.waitIdle();
    }

    void Demo::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory)
    {
        vk::BufferCreateInfo bufferInfo{ {}, size, usage };
        buffer = static_cast<vk::Device>(m_device).createBufferUnique(bufferInfo);

        const auto memRequirements{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*buffer) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, m_instance.getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, properties) };
        bufferMemory = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);

        static_cast<vk::Device>(m_device).bindBufferMemory(*buffer, *bufferMemory, 0);
    }

    void Demo::timing(const bool print)
    {
        const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_timepoint);
        m_elapsedTime += microseconds;
        ++m_timepointCount;
        if (m_elapsedTime.count() > 1e6)
        {
            m_avgFrameTime = static_cast<double>(m_elapsedTime.count()) / static_cast<double>(m_timepointCount);
            m_avgFps = 1e6 / m_avgFrameTime;
            if (print)
            {
                std::cout << "Avg frametime: " << m_avgFrameTime << " microseconds. Avg FPS: " << m_avgFps << " fps\n";
            }

            m_elapsedTime = std::chrono::microseconds::zero();
            m_timepointCount = 0;
        }

        m_timepoint = std::chrono::steady_clock::now();
    }
}
