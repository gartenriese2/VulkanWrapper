#include "window.hpp"

#include <algorithm>
#include <tuple>

namespace vw::util
{
    constexpr int32_t k_minWidth{ 320 };
    constexpr int32_t k_minHeight{ 240 };
    constexpr int32_t k_maxWidth{ 2560 };
    constexpr int32_t k_maxHeight{ 1440 };

    void errorFun(int code, const char * description)
    {
        throw std::runtime_error("A GLFW error occured (code: " + std::to_string(code) + ", description: " + description + ")");
    }

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

        glfwSetErrorCallback(errorFun);

        glfwSetWindowUserPointer(m_window.get(), this);
        glfwSetWindowSizeCallback(m_window.get(), [](GLFWwindow * windowPtr, int width, int height)
        {
            for (const auto & func : static_cast<Window *>(glfwGetWindowUserPointer(windowPtr))->m_windowSizeFuncs)
            {
                func(width, height);
            }
        });
        glfwSetKeyCallback(m_window.get(), [](GLFWwindow * windowPtr, int key, int scancode, int action, int mods)
        {
            for (const auto & func : static_cast<Window *>(glfwGetWindowUserPointer(windowPtr))->m_keyFuncs)
            {
                func(key, scancode, action, mods);
            }
        });
        glfwSetMouseButtonCallback(m_window.get(), [](GLFWwindow * windowPtr, int button, int action, int mods)
        {
            for (const auto & func : static_cast<Window *>(glfwGetWindowUserPointer(windowPtr))->m_mouseButtonFuncs)
            {
                func(button, action, mods);
            }
        });
        glfwSetScrollCallback(m_window.get(), [](GLFWwindow * windowPtr, double xoffset, double yoffset)
        {
            for (const auto & func : static_cast<Window *>(glfwGetWindowUserPointer(windowPtr))->m_mouseScrollFuncs)
            {
                func(xoffset, yoffset);
            }
        });
        glfwSetCursorPosCallback(m_window.get(), [](GLFWwindow * windowPtr, double xpos, double ypos)
        {
           for (const auto & func : static_cast<Window *>(glfwGetWindowUserPointer(windowPtr))->m_mouseCursorFuncs)
           {
               func(xpos, ypos);
           }
        });
    }

    Window::~Window()
    {
        m_window.reset(nullptr);
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

    std::tuple<int32_t, int32_t> Window::getFramebufferSize() const
    {
        int width, height;
        glfwGetFramebufferSize(m_window.get(), &width, &height);
        return std::make_tuple(width, height);
    }

    bool Window::isFocused() const
    {
        return glfwGetWindowAttrib(m_window.get(), GLFW_FOCUSED);
    }

    std::tuple<double, double> Window::getCursorPos() const
    {
        double x, y;
        glfwGetCursorPos(m_window.get(), &x, &y);
        return std::make_tuple(x, y);
    }

    int32_t Window::getMouseButtonState(const int32_t button) const
    {
        return glfwGetMouseButton(m_window.get(), button);
    }

    void Window::setInputMode(const int32_t mode, const int32_t value) const
    {
        glfwSetInputMode(m_window.get(), mode, value);
    }

    void Window::addWindowSizeFunc(const std::function<void(int, int)> & func)
    {
        m_windowSizeFuncs.emplace_back(func);
    }

    void Window::addKeyFunc(const std::function<void(int, int, int, int)> & func)
    {
        m_keyFuncs.emplace_back(func);
    }

    void Window::addMouseButtonFunc(const std::function<void(int, int, int)> & func)
    {
        m_mouseButtonFuncs.emplace_back(func);
    }

    void Window::addMouseScrollFunc(const std::function<void(double, double)> & func)
    {
        m_mouseScrollFuncs.emplace_back(func);
    }

    void Window::addMouseCursorFunc(const std::function<void(double, double)> & func)
    {
        m_mouseCursorFuncs.emplace_back(func);
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
        VkSurfaceKHR surface_vk;
        if (glfwCreateWindowSurface(instance_vk, m_window.get(), nullptr, &surface_vk) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }

        vk::SurfaceKHR surface(surface_vk);
        vk::ObjectDeleter<vk::Instance> deleter(*instance);
        vk::UniqueHandle<vk::SurfaceKHR> uniqueHandle(surface, deleter);
        return std::move(uniqueHandle);
    }
}
