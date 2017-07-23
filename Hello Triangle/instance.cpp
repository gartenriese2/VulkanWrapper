#include <iostream>
#include "instance.hpp"
#include "vulkan_ext.h"

namespace bmvk
{
    constexpr auto standardValidationLayerName = "VK_LAYER_LUNARG_standard_validation";
    constexpr auto debugExtensionName = "VK_EXT_debug_report";

    Instance::Instance(const std::string& appName, const uint32_t appVersion, const std::string& engineName, const uint32_t engineVersion, const std::unique_ptr<Window> & windowPtr, const bool enableValidationLayers)
    {
        vk::ApplicationInfo appInfo{ appName.c_str(), appVersion, engineName.c_str(), engineVersion, VK_API_VERSION_1_0 };

        const auto extensions = getExtensions(enableValidationLayers, windowPtr);
        std::vector<const char*> extensionsAsCstrings{};
        for (const auto& string : extensions)
        {
            extensionsAsCstrings.emplace_back(string.c_str());
        }

        const auto layers = getLayers(enableValidationLayers);
        std::vector<const char*> layersAsCstrings;
        for (const auto& string : layers)
        {
            layersAsCstrings.emplace_back(string.c_str());
        }

        vk::InstanceCreateInfo info{ vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layersAsCstrings.size()), layersAsCstrings.data(), static_cast<uint32_t>(extensionsAsCstrings.size()), extensionsAsCstrings.data()};
        m_instance = vk::createInstance(info);

        vkExtInitInstance(getCInstance());
    }

    Instance::~Instance()
    {
        m_instance.destroy();
    }

    std::vector<std::string> Instance::getExtensions(const bool enableValidationLayers, const std::unique_ptr<Window> & windowPtr) const
    {
        const auto availableExtensions{ vk::enumerateInstanceExtensionProperties() };

        if (enableValidationLayers)
        {
            std::cout << "available extensions:" << std::endl;
            for (const auto & extension : availableExtensions) {
                std::cout << "\t" << extension.extensionName << std::endl;
            }
        }

        const auto glfwExtensions = windowPtr->getRequiredExtensions();
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
                if (strcmp(extension.extensionName, debugExtensionName) == 0)
                {
                    hasDebugExtension = true;
                    break;
                }
            }

            if (!hasDebugExtension)
            {
                throw std::runtime_error("debug extension not available!");
            }

            extensions.emplace_back(debugExtensionName);
        }

        return extensions;
    }

    std::vector<std::string> Instance::getLayers(const bool enableValidationLayers) const
    {
        std::vector<std::string> layers;
        if (!enableValidationLayers)
        {
            return layers;
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
            if (strcmp(prop.layerName, standardValidationLayerName) == 0)
            {
                hasValidationLayer = true;
                break;
            }
        }

        if (!hasValidationLayer)
        {
            throw std::runtime_error(static_cast<std::string>(standardValidationLayerName) + " not available!");
        }

        layers.emplace_back(standardValidationLayerName);

        return layers;
    }
} // namespace bmvk