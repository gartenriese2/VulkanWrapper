#include "instance.hpp"

#include <iostream>

#include <vw/window.hpp>

#include "vulkan_ext.h"

namespace bmvk
{
    constexpr auto k_standardValidationLayerName = "VK_LAYER_LUNARG_standard_validation";
    constexpr auto k_assistantLayerName = "VK_LAYER_LUNARG_assistant_layer";
    constexpr auto k_debugExtensionName = "VK_EXT_debug_report";

    Instance::Instance(const std::string& appName, const uint32_t appVersion, const std::string& engineName, const uint32_t engineVersion, const vw::util::Window & window, const bool enableValidationLayers, const DebugReport::ReportLevel reportLevel)
    {
        vk::ApplicationInfo appInfo{ appName.c_str(), appVersion, engineName.c_str(), engineVersion, VK_API_VERSION_1_0 };

        const auto extensions = getExtensions(enableValidationLayers, window);
        std::vector<const char*> extensionsAsCstrings{};
        for (const auto& string : extensions)
        {
            extensionsAsCstrings.emplace_back(string.c_str());
        }

        initializeLayerNames(enableValidationLayers);

        InstanceCreateInfo info{ {}, &appInfo, m_layerNames, extensionsAsCstrings };
        m_instance = vk::createInstanceUnique(info);

        vkExtInitInstance(getCInstance());

        if (enableValidationLayers)
        {
            const auto flags{ reportLevel == DebugReport::ReportLevel::WarningsAndAbove ? vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning : vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::ePerformanceWarning };
            m_debugReportPtr = std::make_unique<DebugReport>(*this, flags);
        }
        
        m_surface = Surface(std::move(window.createSurface(m_instance)));
        m_physicalDevice = getSuitablePhysicalDevice(reinterpret_cast<const vk::UniqueSurfaceKHR &>(m_surface));
    }

    PhysicalDevice Instance::getSuitablePhysicalDevice(const vk::UniqueSurfaceKHR & surface) const
    {
        const auto physicalDevices = m_instance->enumeratePhysicalDevices();

        auto foundSuitablePhysicalDevice{ false };
        vk::PhysicalDevice chosenPhysicalDevice;
        uint32_t chosenIndex;
        for (auto physicalDevice : physicalDevices)
        {
            bool isSuitable;
            int queueFamilyIndex;
            std::tie(isSuitable, queueFamilyIndex) = PhysicalDevice::isDeviceSuitable(physicalDevice, surface);
            if (isSuitable)
            {
                foundSuitablePhysicalDevice = true;
                chosenPhysicalDevice = physicalDevice;
                chosenIndex = queueFamilyIndex;
                break;
            }
        }

        if (!foundSuitablePhysicalDevice)
        {
            throw std::runtime_error("No suitable physical device found");
        }

        return PhysicalDevice(chosenPhysicalDevice, chosenIndex);
    }

    std::vector<std::string> Instance::getExtensions(const bool enableValidationLayers, const vw::util::Window & window) const
    {
        const auto availableExtensions{ vk::enumerateInstanceExtensionProperties() };

        if (enableValidationLayers)
        {
            std::cout << "available extensions:" << std::endl;
            for (const auto & extension : availableExtensions) {
                std::cout << "\t" << extension.extensionName << std::endl;
            }
        }

        const auto glfwExtensions = window.getRequiredExtensions();
        for (const auto & neededExtension : glfwExtensions)
        {
            if (std::find_if(availableExtensions.cbegin(), availableExtensions.cend(), [&](const auto & ex) { return ex.extensionName == neededExtension; }) == availableExtensions.cend())
            {
                throw std::runtime_error("required glfw extension (" + neededExtension + ") not available!");
            }
        }

        std::vector<std::string> extensions{};
        for (const auto string : glfwExtensions)
        {
            extensions.emplace_back(string);
        }

        if (enableValidationLayers)
        {
            auto hasDebugExtension{ false };
            for (const auto & extension : availableExtensions)
            {
                if (strcmp(extension.extensionName, k_debugExtensionName) == 0)
                {
                    hasDebugExtension = true;
                    break;
                }
            }

            if (!hasDebugExtension)
            {
                throw std::runtime_error("debug extension not available!");
            }

            extensions.emplace_back(k_debugExtensionName);
        }

        return extensions;
    }

    void Instance::initializeLayerNames(const bool enableValidationLayers)
    {
        if (!enableValidationLayers)
        {
            return;
        }

        const auto availableLayers = vk::enumerateInstanceLayerProperties();

        std::cout << "available layers:" << std::endl;
        for (const auto & layer : availableLayers)
        {
            std::cout << "\t" << layer.layerName << std::endl;
        }

        auto hasValidationLayer{ false };
        auto hasAssistantLayer{ false };
        for (const auto & prop : availableLayers)
        {
            if (strcmp(prop.layerName, k_standardValidationLayerName) == 0)
            {
                hasValidationLayer = true;
                continue;
            }

            if (strcmp(prop.layerName, k_assistantLayerName) == 0)
            {
                hasAssistantLayer = true;
            }
        }

        if (!hasValidationLayer)
        {
            throw std::runtime_error(static_cast<std::string>(k_standardValidationLayerName) + " not available!");
        }

        m_layerNames.emplace_back(k_standardValidationLayerName);
        
        if (!hasAssistantLayer)
        {
            std::cout << "Assistant layer is not available, continuing without it ...\n";
        }
        else
        {
            m_layerNames.emplace_back(k_assistantLayerName);
        }
    }
} // namespace bmvk
