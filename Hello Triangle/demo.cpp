#include "demo.hpp"

#include <iostream>

namespace bmvk
{
    Demo::Demo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name)
      : m_window{ width, height, name },
        m_instance{ name, VK_MAKE_VERSION(1, 0, 0), "bmvk", VK_MAKE_VERSION(1, 0, 0), m_window, enableValidationLayers },
        m_device{ m_instance.getPhysicalDevice().createLogicalDevice(m_instance.getLayerNames(), enableValidationLayers) },
        m_timepoint{ std::chrono::steady_clock::now() },
        m_elapsedTime{ std::chrono::microseconds::zero() }
    {
        
    }

    void Demo::timing()
    {
        const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_timepoint);
        m_elapsedTime += microseconds;
        ++m_timepointCount;
        if (m_elapsedTime.count() > 1000000)
        {
            const auto avgFrameTime = static_cast<double>(m_elapsedTime.count()) / static_cast<double>(m_timepointCount);
            const auto avgFps = 1000000.0 / avgFrameTime;
            std::cout << "Avg frametime: " << avgFrameTime << " microseconds. Avg FPS: " << avgFps << " fps\n";
            m_elapsedTime = std::chrono::microseconds::zero();
            m_timepointCount = 0;
        }

        m_timepoint = std::chrono::steady_clock::now();
    }
}
