#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include "instance.hpp"
#include "window.hpp"

constexpr int32_t WIDTH = 800;
constexpr int32_t HEIGHT = 600;

template <typename T>
class VDeleter
{
public:
    VDeleter() : VDeleter([](T, VkAllocationCallbacks *) {}) {}

    VDeleter(std::function<void(T, VkAllocationCallbacks *)> deletef)
    {
        this->deleter = [=](T obj) { deletef(obj, nullptr); };
    }

    VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks *)> deletef)
    {
        this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks *)> deletef)
    {
        this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    ~VDeleter()
    {
        cleanup();
    }

    const T * operator &() const
    {
        return &object;
    }

    T * replace()
    {
        cleanup();
        return &object;
    }

    operator T() const
    {
            return object;
    }

    void operator=(T rhs)
    {
        if (rhs != object)
        {
            cleanup();
            object = rhs;
        }
    }

    template<typename V>
    bool operator==(V rhs)
    {
        return object == T(rhs);
    }

private:

    T object{ VK_NULL_HANDLE };
    std::function<void(T)> deleter;

    void cleanup()
    {
        if (object != VK_NULL_HANDLE)
        {
            deleter(object);
        }

        object = VK_NULL_HANDLE;
    }
};

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
    }

private:
    std::unique_ptr<bmvk::Window> m_windowPtr;
    std::unique_ptr<bmvk::Instance> m_instancePtr;
    
    void initWindow()
    {
        try
        {
            m_windowPtr = std::make_unique<bmvk::Window>(WIDTH, HEIGHT);
        }
        catch (const std::runtime_error & e)
        {
            std::cout << e.what() << '\n';
        }

        std::cout << "Created GLFW window!\n";
    }

    void createInstance()
    {
        try
        {
            m_instancePtr = std::make_unique<bmvk::Instance>("Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "No Engine", VK_MAKE_VERSION(1, 0, 0), m_windowPtr);
        }
        catch (const std::runtime_error & e)
        {
            std::cout << e.what() << '\n';
            return;
        }

        std::cout << "Created instance!\n";
    }

    void initVulkan()
    {
        createInstance();
    }

    void mainLoop()
    {
        while (!m_windowPtr->shouldClose())
        {
            m_windowPtr->pollEvents();
        }
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::runtime_error & e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}