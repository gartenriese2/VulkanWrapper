#pragma once

#include <vulkan/vulkan.hpp>
#include "physicalDevice.hpp"
#include "surface.hpp"
#include "debugReport.hpp"

namespace vw
{
    namespace util
    {
        class Window;
    }
}

namespace bmvk
{
    class Instance
    {
    public:
        Instance(const std::string & appName, const uint32_t appVersion, const std::string & engineName, const uint32_t engineVersion, const vw::util::Window & window, const bool enableValidationLayers, const bool onlyWarningsAndAbove = false);
        Instance(const Instance &) = delete;
        Instance(Instance && other) = default;
        Instance & operator=(const Instance &) = delete;
        Instance & operator=(Instance &&) & = default;
        ~Instance() {}

        auto & getInstance() noexcept { return m_instance; }
        const auto & getInstance() const noexcept { return m_instance; }
        auto getCInstance() noexcept { return static_cast<VkInstance>(*m_instance); }
        auto getCInstance() const noexcept { return static_cast<VkInstance>(*m_instance); }

        const auto & getSurface() const noexcept { return m_surface; }
        const auto & getPhysicalDevice() const noexcept { return m_physicalDevice; }

        const auto & getLayerNames() noexcept { return m_layerNames; }

        PhysicalDevice getSuitablePhysicalDevice(const vk::SurfaceKHR & surface) const;
    private:
        vk::UniqueInstance m_instance;
        std::unique_ptr<DebugReport> m_debugReportPtr;
        Surface m_surface;
        PhysicalDevice m_physicalDevice;
        std::vector<const char*> m_layerNames;

        std::vector<std::string> getExtensions(const bool enableValidationLayers, const vw::util::Window & window) const;
        void initializeLayerNames(const bool enableValidationLayers);
    };

    static_assert(std::is_move_constructible_v<Instance>);
    static_assert(!std::is_copy_constructible_v<Instance>);
    static_assert(std::is_move_assignable_v<Instance>);
    static_assert(!std::is_copy_assignable_v<Instance>);
}
