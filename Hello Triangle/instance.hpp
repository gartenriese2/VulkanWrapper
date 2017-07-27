#pragma once

#include <vulkan/vulkan.hpp>
#include "physicalDevice.hpp"

namespace bmvk
{
    class Window;

    class Instance
    {
    public:
        Instance(const std::string & appName, const uint32_t appVersion, const std::string & engineName, const uint32_t engineVersion, const Window & window, const bool enableValidationLayers);
        Instance(const Instance &) = delete;
        Instance(Instance && other) = default;
        Instance & operator=(const Instance &) = delete;
        Instance & operator=(Instance &&) & = default;
        ~Instance();

        auto & getInstance() noexcept { return m_instance; }
        const auto & getInstance() const noexcept { return m_instance; }
        auto getCInstance() noexcept { return static_cast<VkInstance>(m_instance); }
        auto getCInstance() const noexcept { return static_cast<VkInstance>(m_instance); }
        const auto & getLayerNames() noexcept { return m_layerNames; }

        PhysicalDevice getSuitablePhysicalDevice() const;
    private:
        vk::Instance m_instance;
        std::vector<const char*> m_layerNames;

        std::vector<std::string> getExtensions(const bool enableValidationLayers, const Window & window) const;
        void initializeLayerNames(const bool enableValidationLayers);
    };

    static_assert(std::is_nothrow_move_constructible_v<Instance>);
    static_assert(!std::is_copy_constructible_v<Instance>);
    static_assert(std::is_nothrow_move_assignable_v<Instance>);
    static_assert(!std::is_copy_assignable_v<Instance>);
}
