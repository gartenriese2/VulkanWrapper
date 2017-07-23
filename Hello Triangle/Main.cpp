//template <typename T>
//class VDeleter
//{
//public:
//    VDeleter() : VDeleter([](T, VkAllocationCallbacks *) {}) {}
//
//    VDeleter(std::function<void(T, VkAllocationCallbacks *)> deletef)
//    {
//        this->deleter = [=](T obj) { deletef(obj, nullptr); };
//    }
//
//    VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks *)> deletef)
//    {
//        this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
//    }
//
//    VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks *)> deletef)
//    {
//        this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
//    }
//
//    ~VDeleter()
//    {
//        cleanup();
//    }
//
//    const T * operator &() const
//    {
//        return &object;
//    }
//
//    T * replace()
//    {
//        cleanup();
//        return &object;
//    }
//
//    operator T() const
//    {
//            return object;
//    }
//
//    void operator=(T rhs)
//    {
//        if (rhs != object)
//        {
//            cleanup();
//            object = rhs;
//        }
//    }
//
//    template<typename V>
//    bool operator==(V rhs)
//    {
//        return object == T(rhs);
//    }
//
//private:
//
//    T object{ VK_NULL_HANDLE };
//    std::function<void(T)> deleter;
//
//    void cleanup()
//    {
//        if (object != VK_NULL_HANDLE)
//        {
//            deleter(object);
//        }
//
//        object = VK_NULL_HANDLE;
//    }
//};

#include <iostream>
#include <stdexcept>

#include "window.hpp"
#include "instance.hpp"
#include "debugReport.hpp"
#include "physicalDevice.hpp"
#include "device.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class HelloTriangleApplication {
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
    std::unique_ptr<bmvk::DebugReport> m_debugReportPtr;
    std::unique_ptr<bmvk::PhysicalDevice> m_physicalDevicePtr;
    std::unique_ptr<bmvk::Device> m_devicePtr;

    void initWindow()
    {
        try
        {
            m_windowPtr = std::make_unique<bmvk::Window>(WIDTH, HEIGHT);
        }
        catch (const std::runtime_error & e)
        {
            std::cerr << e.what() << '\n';
            throw;
        }
            
        std::cout << "Created GLFW window!\n";
    }

    void initVulkan()
    {
        createInstance();
        if (enableValidationLayers)
        {
            setupDebugReport();
        }

        pickPhysicalDevice();
        createLogicalDevice();
    }

    void mainLoop()
    {
        while (!m_windowPtr->shouldClose())
        {
            m_windowPtr->pollEvents();
        }
    }

    void createInstance()
    {
        try
        {
            m_instancePtr = std::make_unique<bmvk::Instance>("Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "bmvk", VK_MAKE_VERSION(1, 0, 0), m_windowPtr, enableValidationLayers);
        }
        catch (const std::runtime_error & e)
        {
            std::cerr << e.what() << '\n';
            throw;
        }
            
        std::cout << "Created instance!\n";
    }

    void setupDebugReport()
    {
        try
        {
            m_debugReportPtr = std::make_unique<bmvk::DebugReport>(m_instancePtr, vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eInformation);
        }
        catch (const std::runtime_error & e)
        {
            std::cerr << e.what() << '\n';
            throw;
        }
    }

    void pickPhysicalDevice()
    {
        try
        {
            m_physicalDevicePtr = std::make_unique<bmvk::PhysicalDevice>(m_instancePtr);
        }
        catch (const std::runtime_error & e)
        {
            std::cerr << e.what() << '\n';
            throw;
        }

        std::cout << "Picked physical device!\n";
    }

    void createLogicalDevice() {
        try
        {
            m_devicePtr = m_physicalDevicePtr->createLogicalDevice(m_instancePtr, enableValidationLayers);
        }
        catch (const std::runtime_error & e)
        {
            std::cerr << e.what() << '\n';
            throw;
        }
        
        std::cout << "Created logical device!\n";
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