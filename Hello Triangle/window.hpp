#pragma once

#include <type_traits>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include "surface.hpp"

namespace bmvk
{
    class Instance;

    class Window
    {
    public:
        Window(const int32_t width, const int32_t height);
        Window(const Window &) = delete;
        Window(Window && other) = default;
        Window & operator=(const Window &) = delete;
        Window & operator=(Window &&) & = default;
        ~Window();

        bool shouldClose() const;
        void pollEvents() const;
        std::vector<std::string> getRequiredExtensions() const;
        Surface createSurface(const Instance & instance) const;
    private:
        struct GLFWwindowDeleter {
            void operator()(GLFWwindow * ptr) const
            {
                glfwDestroyWindow(ptr);
            }
        };

        std::unique_ptr<GLFWwindow, GLFWwindowDeleter> m_window;
    };

    static_assert(std::is_nothrow_move_constructible_v<Window>);
    static_assert(!std::is_copy_constructible_v<Window>);
    static_assert(std::is_nothrow_move_assignable_v<Window>);
    static_assert(!std::is_copy_assignable_v<Window>);
}
