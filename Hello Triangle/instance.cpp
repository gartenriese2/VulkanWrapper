#include <iostream>
#include "instance.hpp"
#include "vulkan_ext.h"
#include "window.hpp"

namespace bmvk
{
    constexpr auto k_standardValidationLayerName = "VK_LAYER_LUNARG_standard_validation";
    constexpr auto k_debugExtensionName = "VK_EXT_debug_report";

    Instance::Instance(const std::string& appName, const uint32_t appVersion, const std::string& engineName, const uint32_t engineVersion, const Window & window, const bool enableValidationLayers)
    {
        vk::ApplicationInfo appInfo{ appName.c_str(), appVersion, engineName.c_str(), engineVersion, VK_API_VERSION_1_0 };

        const auto extensions = getExtensions(enableValidationLayers, window);
        std::vector<const char*> extensionsAsCstrings{};
        for (const auto& string : extensions)
        {
            extensionsAsCstrings.emplace_back(string.c_str());
        }

        initializeLayerNames(enableValidationLayers);

        vk::InstanceCreateInfo info{ vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(m_layerNames.size()), m_layerNames.data(), static_cast<uint32_t>(extensionsAsCstrings.size()), extensionsAsCstrings.data()};
        m_instance = vk::createInstance(info);

        vkExtInitInstance(getCInstance());
    }

    Instance::~Instance()
    {
        m_instance.destroy();
    }

    PhysicalDevice Instance::getSuitablePhysicalDevice() const
    {
        const auto physicalDevices = m_instance.enumeratePhysicalDevices();

        auto foundSuitablePhysicalDevice{ false };
        vk::PhysicalDevice chosenPhysicalDevice;
        uint32_t chosenIndex;
        for (auto physicalDevice : physicalDevices)
        {
            bool isSuitable;
            int queueFamilyIndex;
            std::tie(isSuitable, queueFamilyIndex) = PhysicalDevice::isDeviceSuitable(physicalDevice);
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

    std::vector<std::string> Instance::getExtensions(const bool enableValidationLayers, const Window & window) const
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
        for (const auto & prop : availableLayers)
        {
            if (strcmp(prop.layerName, k_standardValidationLayerName) == 0)
            {
                hasValidationLayer = true;
                break;
            }
        }

        if (!hasValidationLayer)
        {
            throw std::runtime_error(static_cast<std::string>(k_standardValidationLayerName) + " not available!");
        }

        m_layerNames.emplace_back(k_standardValidationLayerName);
    }
} // namespace bmvk
