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
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
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
    std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;
    vk::UniqueCommandPool m_commandPool;
    std::vector<vk::UniqueCommandBuffer> m_commandBuffers;

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

    void createFramebuffers()
    {
        m_swapChainFramebuffers.resize(m_swapchainImageViews.size());
        for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
        {
            vk::ImageView attachments[] { m_swapchainImageViews[i].get() };
            vk::FramebufferCreateInfo framebufferInfo{ vk::FramebufferCreateFlags(), m_renderPass.get(), 1, attachments, m_swapchainExtent.width, m_swapchainExtent.height, 1 };
            m_swapChainFramebuffers[i] = static_cast<vk::Device>(m_device).createFramebufferUnique(framebufferInfo);
        }
    }

    void createCommandPool()
    {
        auto queueFamilyIndices{ m_instance.getPhysicalDevice().getQueueFamilyIndex() };
        vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlags(), queueFamilyIndices };
        m_commandPool = static_cast<vk::Device>(m_device).createCommandPoolUnique(poolInfo);
    }

    void createCommandBuffers()
    {
        m_commandBuffers.resize(m_swapChainFramebuffers.size());
        vk::CommandBufferAllocateInfo allocInfo{ m_commandPool.get(), vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(m_commandBuffers.size()) };
        m_commandBuffers = static_cast<vk::Device>(m_device).allocateCommandBuffersUnique(allocInfo);
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            m_commandBuffers[i].get().begin(beginInfo);
            vk::ClearValue clearColor{ vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }) };
            vk::RenderPassBeginInfo renderPassInfo{ m_renderPass.get(), m_swapChainFramebuffers[i].get(), vk::Rect2D({0, 0}, m_swapchainExtent), 1, &clearColor };
            m_commandBuffers[i].get().beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
            m_commandBuffers[i].get().bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.get());
            m_commandBuffers[i].get().draw(3, 1, 0, 0);
            m_commandBuffers[i].get().endRenderPass();
            m_commandBuffers[i].get().end();
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