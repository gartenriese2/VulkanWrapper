#pragma once

#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class Surface
    {
    public:
        explicit Surface(vk::SurfaceKHR surface);
        Surface(const Surface &) = delete;
        Surface(Surface && other) = default;
        Surface & operator=(const Surface &) = delete;
        Surface & operator=(Surface && other) = default;
        ~Surface() {}

        explicit operator vk::SurfaceKHR() const noexcept { return m_surface; }
    private:
        vk::SurfaceKHR m_surface;

        friend class Instance;
        Surface() {}
    };
}
