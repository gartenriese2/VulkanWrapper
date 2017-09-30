#include "window.hpp"

#include <algorithm>
#include <tuple>

namespace vw::util
{
    constexpr int32_t k_minWidth{ 320 };
    constexpr int32_t k_minHeight{ 240 };
    constexpr int32_t k_maxWidth{ 2560 };
    constexpr int32_t k_maxHeight{ 1440 };

    Window::Window(const uint32_t width, const uint32_t height, std::string_view name)
    {
        if (glfwInit() == GLFW_FALSE)
        {
            throw std::runtime_error("glfwInit failed!");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        m_window.reset(glfwCreateWindow(std::min(k_maxWidth, std::max(k_minWidth, static_cast<int32_t>(width))), std::min(k_maxHeight, std::max(k_minHeight, static_cast<int32_t>(height))), name.data(), nullptr, nullptr));
        if (m_window.get() == nullptr)
        {
            throw std::runtime_error("glfwCreateWindow failed!");
        }
    }

    Window::~Window()
    {
        glfwTerminate();
    }

    bool Window::shouldClose() const
    {
        return glfwWindowShouldClose(m_window.get());
    }

    void Window::setShouldClose(const bool shouldClose) const
    {
        glfwSetWindowShouldClose(m_window.get(), shouldClose);
    }

    void Window::pollEvents() const
    {
        glfwPollEvents();
    }

    std::tuple<int32_t, int32_t> Window::getSize() const
    {
        int width, height;
        glfwGetWindowSize(m_window.get(), &width, &height);
        return std::make_tuple(width, height);
    }

    void Window::setWindowUserPointer(void * pointer) const
    {
        glfwSetWindowUserPointer(m_window.get(), pointer);
    }

    void Window::setWindowSizeCallback(GLFWwindowsizefun fun) const
    {
        glfwSetWindowSizeCallback(m_window.get(), fun);
    }

    void Window::setKeyCallback(GLFWkeyfun fun) const
    {
        glfwSetKeyCallback(m_window.get(), fun);
    }

    void Window::setMouseButtonCallback(GLFWmousebuttonfun fun) const
    {
        glfwSetMouseButtonCallback(m_window.get(), fun);
    }

    void Window::setCursorPosCallback(GLFWcursorposfun fun) const
    {
        glfwSetCursorPosCallback(m_window.get(), fun);
    }

    std::vector<std::string> Window::getRequiredExtensions() const
    {
        auto glfwExtensionCount = 0u;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        return std::vector<std::string>{ glfwExtensions, glfwExtensions + glfwExtensionCount };
    }

    vk::UniqueSurfaceKHR Window::createSurface(const vk::UniqueInstance & instance) const
    {
        const auto instance_vk{ static_cast<VkInstance>(*instance) };
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance_vk, m_window.get(), nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }

        return vk::UniqueSurfaceKHR{ static_cast<vk::SurfaceKHR>(surface), vk::SurfaceKHRDeleter{ *instance } };
    }
}
