#include "demo.hpp"

namespace bmvk
{
    Demo::Demo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name)
      : m_window{ width, height, name },
        m_instance{ name, VK_MAKE_VERSION(1, 0, 0), "bmvk", VK_MAKE_VERSION(1, 0, 0), m_window, enableValidationLayers },
        m_device{ m_instance.getPhysicalDevice().createLogicalDevice(m_instance.getLayerNames(), enableValidationLayers) }
    {
        
    }

}
