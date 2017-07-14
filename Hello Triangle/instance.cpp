#include "instance.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

namespace bmvk
{
    Instance::Instance(const std::string& appName, const uint32_t appVersion, const std::string& engineName, const uint32_t engineVersion, const std::unique_ptr<Window> & windowPtr)
    {
        vk::ApplicationInfo appInfo{ appName.c_str(), appVersion, engineName.c_str(), engineVersion, VK_API_VERSION_1_0 };

        const auto availableExtensions{ vk::enumerateInstanceExtensionProperties() };

        std::cout << "available extensions:" << std::endl;
        for (const auto & extension : availableExtensions) {
            std::cout << "\t" << extension.extensionName << std::endl;
        }

        const auto glfwExtensions = windowPtr->getRequiredExtensions();
        for (const auto & neededExtension : glfwExtensions)
        {
            if (std::find_if(availableExtensions.cbegin(), availableExtensions.cend(), [&](const auto & ex) { return ex.extensionName == neededExtension; }) == availableExtensions.cend())
            {
                throw std::runtime_error("required glfw extension (" + neededExtension + ") not available!");
            }
        }

        std::vector<const char*> cstrings{};
        for (const auto& string : glfwExtensions) {
            cstrings.push_back(string.c_str());
        }

        vk::InstanceCreateInfo info{ vk::InstanceCreateFlags(), &appInfo, 0, nullptr, static_cast<uint32_t>(glfwExtensions.size()), cstrings.data()};
        m_instance = vk::createInstance(info);
    }

    Instance::~Instance()
    {
        m_instance.destroy();
    }
} // namespace bmvk
