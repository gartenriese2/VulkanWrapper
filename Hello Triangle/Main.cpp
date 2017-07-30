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
        m_device{ m_instance.getPhysicalDevice().createLogicalDevice(m_instance.getLayerNames(), enableValidationLayers) },
        m_queue{ m_device.createQueue() }
    {
        createSwapchain();
        createGraphicsPipeline();
    }

    void run()
    {
        mainLoop();
    }

private:
    bmvk::Window m_window;
    bmvk::Instance m_instance;
    bmvk::Device m_device;
    bmvk::Queue m_queue;
    vk::SurfaceFormatKHR m_swapchainImageFormat;
    vk::Extent2D m_swapchainExtent;
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::UniqueImageView> m_swapchainImageViews;

    void mainLoop()
    {
        while (!m_window.shouldClose())
        {
            m_window.pollEvents();
        }
    }

    void createSwapchain()
    {
        const auto capabilities{ m_instance.getPhysicalDevice().getSurfaceCapabilities(m_instance.getSurface().getSurface()) };
        const auto formats{ m_instance.getPhysicalDevice().getSurfaceFormats(m_instance.getSurface().getSurface()) };
        const auto presentModes{ m_instance.getPhysicalDevice().getPresentModes(m_instance.getSurface().getSurface()) };
        m_swapchainImageFormat = bmvk::PhysicalDevice::chooseSwapSurfaceFormat(formats);
        m_swapchainExtent = bmvk::PhysicalDevice::chooseSwapExtent(capabilities, WIDTH, HEIGHT);
        m_device.createSwapchain(
            m_instance.getSurface().getSurface(),
            bmvk::PhysicalDevice::chooseImageCount(capabilities),
            m_swapchainImageFormat,
            m_swapchainExtent,
            capabilities,
            bmvk::PhysicalDevice::chooseSwapPresentMode(presentModes));
        m_swapchainImages = m_device.getSwapchainImages();
        m_swapchainImageViews.resize(m_swapchainImages.size());
        for (size_t i = 0; i < m_swapchainImages.size(); i++) {
            const auto info = vk::ImageViewCreateInfo{ vk::ImageViewCreateFlags(), m_swapchainImages[i], vk::ImageViewType::e2D, m_swapchainImageFormat.format, vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1) };
            m_swapchainImageViews[i] = m_device.createImageView(info);
        }
    }

    void createGraphicsPipeline()
    {

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