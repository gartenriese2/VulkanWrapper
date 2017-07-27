#include "debugReport.hpp"
#include "instance.hpp"
#include <iostream>

namespace bmvk
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
    {
        std::string messageString;
        std::string pLayerPrefixString(layerPrefix);

        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            messageString += "ERROR: [" + pLayerPrefixString + "] Code " + std::to_string(code) + " : " + msg;
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            messageString += "WARNING: [" + pLayerPrefixString + "] Code " + std::to_string(code) + " : " + msg;
        }
        else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
            messageString += "INFORMATION: [" + pLayerPrefixString + "] Code " + std::to_string(code) + " : " + msg;
        }
        else
        {
            return VK_FALSE;
        }

        std::cout << messageString << std::endl;

        return VK_FALSE;
    }

    DebugReport::DebugReport(const Instance & instance, vk::DebugReportFlagsEXT flags, const bool enableValidationLayers)
    {
        static_assert(std::is_move_constructible<vk::DebugReportFlagsEXT>());

        if (!enableValidationLayers)
        {
            return;
        }

        vk::DebugReportCallbackCreateInfoEXT createInfo(std::move(flags), debugCallback);
        m_uniqueCallback = std::move(instance.getInstance().createDebugReportCallbackEXTUnique(createInfo));

        std::cout << "Created debug report!\n";
    }
}