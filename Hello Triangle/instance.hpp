#pragma once

#include <vulkan/vulkan.hpp>
#include "window.hpp"

namespace bmvk
{
    class Instance
    {
    public:
        Instance(const std::string & appName, const uint32_t appVersion, const std::string & engineName, const uint32_t engineVersion, const std::unique_ptr<Window> & windowPtr, const bool enableValidationLayers);
        Instance(const Instance &) = delete;
        Instance(Instance && other) = default;
        Instance & operator=(const Instance &) = delete;
        Instance & operator=(Instance &&) & = default;
        ~Instance();

        auto & getInstance() noexcept { return m_instance; }
        auto getCInstance() noexcept { return static_cast<VkInstance>(m_instance); }
        const auto & getLayerNames() noexcept { return m_layerNames; }
    private:
        vk::Instance m_instance;
        std::vector<const char*> m_layerNames;

        std::vector<std::string> getExtensions(const bool enableValidationLayers, const std::unique_ptr<Window> & windowPtr) const;
        void initializeLayerNames(const bool enableValidationLayers);
    };

    static_assert(std::is_nothrow_move_constructible_v<Instance>);
    static_assert(!std::is_copy_constructible_v<Instance>);
    static_assert(std::is_nothrow_move_assignable_v<Instance>);
    static_assert(!std::is_copy_assignable_v<Instance>);
}
