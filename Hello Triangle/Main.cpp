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
    HelloTriangleApplication()
      : m_window{ WIDTH, HEIGHT },
        m_instance{ "Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "bmvk", VK_MAKE_VERSION(1, 0, 0), m_window, enableValidationLayers },
        m_debugReport{ m_instance, vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eInformation, enableValidationLayers },
        m_surface{ m_window.createSurface(m_instance) },
        m_physicalDevice{ m_instance.getSuitablePhysicalDevice(m_surface.getSurface()) },
        m_device{ m_physicalDevice.createLogicalDevice(m_instance.getLayerNames(), enableValidationLayers) },
        m_queue{ m_device.createQueue() }
    {
    }

    void run()
    {
        mainLoop();
    }

private:
    bmvk::Window m_window;
    bmvk::Instance m_instance;
    bmvk::DebugReport m_debugReport;
    bmvk::Surface m_surface;
    bmvk::PhysicalDevice m_physicalDevice;
    bmvk::Device m_device;
    bmvk::Queue m_queue;

    void mainLoop()
    {
        while (!m_window.shouldClose())
        {
            m_window.pollEvents();
        }
    }
};

int main()
{
    try
    {
        HelloTriangleApplication app;
        app.run();
    }
    catch (const std::runtime_error & e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}