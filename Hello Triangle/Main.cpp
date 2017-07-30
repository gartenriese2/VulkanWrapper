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
#include <fstream>

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

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

class HelloTriangleApplication {
public:
    HelloTriangleApplication()
      : m_window{ WIDTH, HEIGHT },
        m_instance{ "Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "bmvk", VK_MAKE_VERSION(1, 0, 0), m_window, enableValidationLayers },
        m_device{ m_instance.getPhysicalDevice().createLogicalDevice(m_instance.getLayerNames(), enableValidationLayers) },
        m_queue{ m_device.createQueue() }
    {
        createSwapchain();
        createRenderPass();
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
    vk::UniqueRenderPass m_renderPass;
    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_graphicsPipeline;

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

    void createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ vk::AttachmentDescriptionFlags(), m_swapchainImageFormat.format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::SubpassDescription subpass{ vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };
        vk::RenderPassCreateInfo renderPassInfo{ vk::RenderPassCreateFlags(), 1, &colorAttachment, 1, &subpass };
        m_renderPass = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);
    }

    void createGraphicsPipeline()
    {
        const auto vertShaderCode{ readFile("../shaders/triangleshader.vert.spv") };
        const auto fragShaderCode{ readFile("../shaders/triangleshader.frag.spv") };

        const auto vertShaderModule{ createShaderModule(vertShaderCode) };
        const auto fragShaderModule{ createShaderModule(fragShaderCode) };

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vertShaderModule.get(), "main" };
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, fragShaderModule.get(), "main" };

        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList };
        vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.f, 1.f };
        vk::Rect2D scissor{ vk::Offset2D(), m_swapchainExtent };
        vk::PipelineViewportStateCreateInfo viewportState{ vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor };
        vk::PipelineRasterizationStateCreateInfo rasterizer{ vk::PipelineRasterizationStateCreateFlags(), false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.f, 0.f, 0.f, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisampling;
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{ false, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlending{ vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        m_pipelineLayout = static_cast<vk::Device>(m_device).createPipelineLayoutUnique(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo{ vk::PipelineCreateFlags(), 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending, nullptr, m_pipelineLayout.get(), m_renderPass.get(), 0, nullptr, -1 };
        m_graphicsPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, pipelineInfo);
    }

    vk::UniqueShaderModule createShaderModule(const std::vector<char> & code) const
    {
        vk::ShaderModuleCreateInfo info{ vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t *>(code.data()) };
        return static_cast<vk::Device>(m_device).createShaderModuleUnique(info);
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