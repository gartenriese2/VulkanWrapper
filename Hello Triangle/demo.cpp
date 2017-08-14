#include "demo.hpp"

#include <iostream>

namespace bmvk
{
    Demo::Demo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name)
      : m_window{width, height, name},
        m_instance{name, VK_MAKE_VERSION(1, 0, 0), "bmvk", VK_MAKE_VERSION(1, 0, 0), m_window, enableValidationLayers},
        m_device{m_instance.getPhysicalDevice().createLogicalDevice(m_instance.getLayerNames(), enableValidationLayers)},
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

        const auto memRequirements{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(buffer.get()) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, m_instance.getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, properties) };
        bufferMemory = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);

        static_cast<vk::Device>(m_device).bindBufferMemory(buffer.get(), bufferMemory.get(), 0);
    }

    void Demo::timing()
    {
        const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_timepoint);
        m_elapsedTime += microseconds;
        ++m_timepointCount;
        if (m_elapsedTime.count() > 1e6)
        {
            const auto avgFrameTime = static_cast<double>(m_elapsedTime.count()) / static_cast<double>(m_timepointCount);
            const auto avgFps = 1e6 / avgFrameTime;
            std::cout << "Avg frametime: " << avgFrameTime << " microseconds. Avg FPS: " << avgFps << " fps\n";
            m_elapsedTime = std::chrono::microseconds::zero();
            m_timepointCount = 0;
        }

        m_timepoint = std::chrono::steady_clock::now();
    }
}
