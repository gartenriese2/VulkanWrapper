#pragma once

#include <vulkan/vulkan.hpp>
#include "window.hpp"

namespace bmvk
{
    class Instance
    {
    public:
        Instance(const std::string & appName, const uint32_t appVersion, const std::string & engineName, const uint32_t engineVersion, const std::unique_ptr<Window> & windowPtr);
        Instance(const Instance &) = delete;
        Instance(Instance && other) = default;
        Instance & operator=(const Instance &) = delete;
        Instance & operator=(Instance &&) & = default;
        ~Instance();
    private:
        vk::Instance m_instance;
    };

    static_assert(std::is_nothrow_move_constructible<Instance>());
    static_assert(!std::is_copy_constructible<Instance>());
    static_assert(std::is_nothrow_move_assignable<Instance>());
    static_assert(!std::is_copy_assignable<Instance>());
}
