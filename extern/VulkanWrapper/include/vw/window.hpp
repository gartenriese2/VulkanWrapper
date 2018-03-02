#pragma once

#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <type_traits>
#include <string_view>
#include <memory>
#include <vector>
#include <functional>

namespace vw::util
{
    class Window
    {
    public:
        Window(const uint32_t width, const uint32_t height, std::string_view name);
        Window(const Window &) = delete;
        Window(Window && other) = default;
        Window & operator=(const Window &) = delete;
        Window & operator=(Window &&) & = default;
        ~Window();

        bool shouldClose() const;
        void setShouldClose(const bool shouldClose) const;
        void pollEvents() const;
        std::tuple<int32_t, int32_t> getSize() const;
        std::tuple<int32_t, int32_t> getFramebufferSize() const;
        bool isFocused() const;
        std::tuple<double, double> getCursorPos() const;
        int32_t getMouseButtonState(const int32_t button) const;
        void setInputMode(const int32_t mode, const int32_t value) const;
        void addWindowSizeFunc(const std::function<void(int, int)> & func);
        void addKeyFunc(const std::function<void(int, int, int, int)> & func);
        void addMouseButtonFunc(const std::function<void(int, int, int)> & func);
        void addMouseScrollFunc(const std::function<void(double, double)> & func);
        void addMouseCursorFunc(const std::function<void(double, double)> & func);
        std::vector<std::string> getRequiredExtensions() const;
        vk::UniqueSurfaceKHR createSurface(const vk::UniqueInstance & instance) const;
    private:
        struct GLFWwindowDeleter {
            void operator()(GLFWwindow * ptr) const
            {
                glfwDestroyWindow(ptr);
            }
        };

        std::unique_ptr<GLFWwindow, GLFWwindowDeleter> m_window;

        std::vector<std::function<void(int, int)>> m_windowSizeFuncs;
        std::vector<std::function<void(int, int, int, int)>> m_keyFuncs;
        std::vector<std::function<void(int, int, int)>> m_mouseButtonFuncs;
        std::vector<std::function<void(double, double)>> m_mouseScrollFuncs;
        std::vector<std::function<void(double, double)>> m_mouseCursorFuncs;
    };

    static_assert(std::is_nothrow_move_constructible_v<Window>);
    static_assert(!std::is_copy_constructible_v<Window>);
    static_assert(std::is_nothrow_move_assignable_v<Window>);
    static_assert(!std::is_copy_assignable_v<Window>);
}
