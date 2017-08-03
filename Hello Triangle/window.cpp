#include "window.hpp"
#include <algorithm>
#include <stdexcept>
#include "instance.hpp"

class HelloTriangleApplication;

namespace bmvk
{
    constexpr int32_t k_minWidth { 320 };
    constexpr int32_t k_minHeight { 240 };

    Window::Window(const int32_t width, const int32_t height)
    {
        if (glfwInit() == GLFW_FALSE)
        {
            throw std::runtime_error("glfwInit failed!");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        m_window.reset(glfwCreateWindow(std::max(k_minWidth, width), std::max(k_minHeight, height), "Vulkan", nullptr, nullptr));
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

    void Window::pollEvents() const
    {
        glfwPollEvents();
    }

    std::tuple<int, int> Window::getSize() const
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

    std::vector<std::string> Window::getRequiredExtensions() const
    {
        auto glfwExtensionCount = 0u;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        return std::vector<std::string>{glfwExtensions, glfwExtensions + glfwExtensionCount};
    }

    Surface Window::createSurface(const Instance & instance) const
    {
        VkSurfaceKHR cSurface;
        if (glfwCreateWindowSurface(instance.getCInstance(), m_window.get(), nullptr, &cSurface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }

        const vk::SurfaceKHR surface{ cSurface };
        return Surface{ surface };
    }
}
