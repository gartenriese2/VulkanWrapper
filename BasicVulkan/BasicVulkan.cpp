#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <array>
#include <set>
#include <unordered_map>

#include "vulkan_ext.h"
#include "../Hello Triangle/sampler.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;

const std::string MODEL_PATH = "../models/stanford_dragon/dragon.obj";
const std::string TEXTURE_PATH = "../textures/chalet.jpg";
const std::string VERTEX_SHADER_PATH = "../shaders/blinnphong.vert.spv";
const std::string FRAGMENT_SHADER_PATH = "../shaders/blinnphong.frag.spv";

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const bool enableCompleteDebugOutput = true;

struct QueueFamilyIndices
{
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() const
    {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, color);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && normal == other.normal && color == other.color && texCoord == other.texCoord;
    }
};

namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1) ^ hash<glm::vec3>()(vertex.normal);
        }
    };
}

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct GLFWwindowDeleter
{
    void operator()(GLFWwindow * ptr) const
    {
        glfwDestroyWindow(ptr);
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
        cleanup();
    }

private:
    std::unique_ptr<GLFWwindow, GLFWwindowDeleter> m_window;

    vk::UniqueInstance m_instance;
    vk::UniqueDebugReportCallbackEXT m_callback;
    vk::UniqueSurfaceKHR m_surface;

    vk::PhysicalDevice m_physicalDevice = nullptr;
    vk::UniqueDevice m_device;

    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;

    vk::UniqueSwapchainKHR m_swapChain;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::UniqueImageView> m_swapChainImageViews;
    std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;

    vk::UniqueRenderPass m_renderPass;
    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_graphicsPipeline;

    vk::UniqueCommandPool m_commandPool;

    vk::UniqueImage m_depthImage;
    vk::UniqueDeviceMemory m_depthImageMemory;
    vk::UniqueImageView m_depthImageView;

    vk::UniqueImage m_textureImage;
    vk::UniqueDeviceMemory m_textureImageMemory;
    vk::UniqueImageView m_textureImageView;
    vk::UniqueSampler m_textureSampler;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    vk::UniqueBuffer m_vertexBuffer;
    vk::UniqueDeviceMemory m_vertexBufferMemory;
    vk::UniqueBuffer m_indexBuffer;
    vk::UniqueDeviceMemory m_indexBufferMemory;

    vk::UniqueBuffer m_uniformBuffer;
    vk::UniqueDeviceMemory m_uniformBufferMemory;

    vk::UniqueDescriptorPool m_descriptorPool;
    vk::UniqueDescriptorSet m_descriptorSet;

    std::vector<vk::UniqueCommandBuffer> m_commandBuffers;

    vk::UniqueSemaphore m_imageAvailableSemaphore;
    vk::UniqueSemaphore m_renderFinishedSemaphore;

    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_window.reset(glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr));

        glfwSetWindowUserPointer(m_window.get(), this);
        glfwSetWindowSizeCallback(m_window.get(), HelloTriangleApplication::onWindowResized);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugCallback();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createDepthResources();
        createFramebuffers();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
        createSemaphores();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(m_window.get())) {
            glfwPollEvents();

            updateUniformBuffer();
            drawFrame();
        }

        m_device->waitIdle();
    }

    void cleanupSwapChain()
    {
        m_depthImageView.reset(nullptr);
        m_depthImage.reset(nullptr);
        m_depthImageMemory.reset(nullptr);

        m_swapChainFramebuffers.clear();

        m_commandBuffers.clear();

        m_graphicsPipeline.reset(nullptr);
        m_pipelineLayout.reset(nullptr);
        m_renderPass.reset(nullptr);

        m_swapChainImageViews.clear();

        m_swapChain.reset(nullptr);
    }

    void cleanup()
    {
        cleanupSwapChain();

        m_textureSampler.reset(nullptr);
        m_textureImageView.reset(nullptr);

        m_textureImage.reset(nullptr);
        m_textureImageMemory.reset(nullptr);

        m_descriptorSet.reset(nullptr);
        m_descriptorPool.reset(nullptr);

        m_descriptorSetLayout.reset(nullptr);
        m_uniformBuffer.reset(nullptr);
        m_uniformBufferMemory.reset(nullptr);

        m_indexBuffer.reset(nullptr);
        m_indexBufferMemory.reset(nullptr);

        m_vertexBuffer.reset(nullptr);
        m_vertexBufferMemory.reset(nullptr);

        m_renderFinishedSemaphore.reset(nullptr);
        m_imageAvailableSemaphore.reset(nullptr);

        m_commandPool.reset(nullptr);

        m_device.reset(nullptr);
        m_callback.reset(nullptr);
        m_surface.reset(nullptr);
        m_instance.reset(nullptr);

        m_window.reset(nullptr);

        glfwTerminate();
    }

    static void onWindowResized(GLFWwindow* window, int width, int height)
    {
        if (width == 0 || height == 0) return;

        auto * app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    void recreateSwapChain()
    {
        m_device->waitIdle();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createDepthResources();
        createFramebuffers();
        createCommandBuffers();
    }

    void createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        vk::ApplicationInfo appInfo{ "Blinn-Phong Demo", VK_MAKE_VERSION(1, 0, 0), "No Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0 };

        const auto extensions = getRequiredExtensions();
        vk::InstanceCreateInfo createInfo{ {}, &appInfo, 0, nullptr, static_cast<uint32_t>(extensions.size()), extensions.data() };
        if (enableValidationLayers) {
            createInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()));
            createInfo.setPpEnabledLayerNames(validationLayers.data());
        }

        m_instance = vk::createInstanceUnique(createInfo);

        auto instance{ *m_instance };
        vkExtInitInstance(static_cast<VkInstance>(instance));
    }

    void setupDebugCallback()
    {
        if (!enableValidationLayers)
        {
            return;
        }

        vk::DebugReportCallbackCreateInfoEXT createInfo{ vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning, debugCallback };
        if (enableCompleteDebugOutput)
        {
            createInfo.flags |= vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::ePerformanceWarning;
        }

        m_callback = m_instance->createDebugReportCallbackEXTUnique(createInfo);
    }

    void createSurface()
    {
        const auto instance{ static_cast<VkInstance>(*m_instance) };
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, m_window.get(), nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }

        vk::UniqueSurfaceKHR s{ static_cast<vk::SurfaceKHR>(surface), vk::SurfaceKHRDeleter{ *m_instance } };
        m_surface = std::move(s);
    }

    void pickPhysicalDevice()
    {
        const auto physicalDevices{ m_instance->enumeratePhysicalDevices() };
        if (physicalDevices.empty()) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        auto found = false;
        for (const auto & device : physicalDevices)
        {
            if (isDeviceSuitable(device))
            {
                m_physicalDevice = device;
                found = true;
                break;
            }
        }

        if (!found)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice()
    {
       const auto indices = findQueueFamilies(m_physicalDevice);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

        auto queuePriority = 1.0f;
        for (auto queueFamily : uniqueQueueFamilies)
        {
            vk::DeviceQueueCreateInfo queueCreateInfo{ {}, static_cast<uint32_t>(queueFamily), 1, &queuePriority };
            queueCreateInfos.emplace_back(queueCreateInfo);
        }

        vk::PhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.setSamplerAnisotropy(true);

        vk::DeviceCreateInfo createInfo{ {}, static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(), 0, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(), &deviceFeatures };
        if (enableValidationLayers)
        {
            createInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()));
            createInfo.setPpEnabledLayerNames(validationLayers.data());
        }

        m_device = m_physicalDevice.createDeviceUnique(createInfo);

        m_graphicsQueue = m_device->getQueue(indices.graphicsFamily, 0);
        m_presentQueue = m_device->getQueue(indices.presentFamily, 0);
    }

    void createSwapChain()
    {
        const auto swapChainSupport = querySwapChainSupport(m_physicalDevice);

        const auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        const auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        const auto extent = chooseSwapExtent(swapChainSupport.capabilities);

        auto imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo{ {}, *m_surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace, extent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, swapChainSupport.capabilities.currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, true, nullptr };

        const auto indices = findQueueFamilies(m_physicalDevice);
        uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.presentFamily) };
        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
            createInfo.setQueueFamilyIndexCount(2);
            createInfo.setPQueueFamilyIndices(queueFamilyIndices);
        }

        m_swapChain = m_device->createSwapchainKHRUnique(createInfo);
        m_swapChainImages = m_device->getSwapchainImagesKHR(*m_swapChain);
        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void createImageViews()
    {
        m_swapChainImageViews.clear();
        for (uint32_t i = 0; i<  m_swapChainImages.size(); ++i)
        {
            m_swapChainImageViews.emplace_back(createImageView(m_swapChainImages[i], m_swapChainImageFormat, vk::ImageAspectFlagBits::eColor));
        }
    }

    void createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ {}, m_swapChainImageFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };
        vk::AttachmentDescription depthAttachment{ {}, findDepthFormat(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal };

        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentReference depthAttachmentRef{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef, nullptr, &depthAttachmentRef };

        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, {} };

        std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        vk::RenderPassCreateInfo renderPassInfo{ {}, static_cast<uint32_t>(attachments.size()), attachments.data(), 1, &subpass, 1, &dependency };

        m_renderPass = m_device->createRenderPassUnique(renderPassInfo);
    }

    void createDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
        vk::DescriptorSetLayoutBinding samplerLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };

        std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        vk::DescriptorSetLayoutCreateInfo layoutInfo{ {}, static_cast<uint32_t>(bindings.size()), bindings.data() };
        m_descriptorSetLayout = m_device->createDescriptorSetLayoutUnique(layoutInfo);
    }

    void createGraphicsPipeline()
    {
        auto vertShaderCode = readFile(VERTEX_SHADER_PATH);
        auto fragShaderCode = readFile(FRAGMENT_SHADER_PATH);

        auto vertShaderModule = createShaderModule(vertShaderCode);
        auto fragShaderModule = createShaderModule(fragShaderCode);

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ {}, vk::ShaderStageFlagBits::eVertex, *vertShaderModule, "main" };
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ {}, vk::ShaderStageFlagBits::eFragment, *fragShaderModule, "main" };

        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ {}, 1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data() };

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList, false };

        vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(m_swapChainExtent.width), static_cast<float>(m_swapChainExtent.height), 0.f, 1.f };
        vk::Rect2D scissor{ { 0, 0 }, m_swapChainExtent };
        vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, &viewport, 1, &scissor };

        vk::PipelineRasterizationStateCreateInfo rasterizer{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, false, 0.f, 0.f, 0.f, 1.f };

        vk::PipelineMultisampleStateCreateInfo multisampling{ {}, vk::SampleCountFlagBits::e1, false };

        vk::PipelineDepthStencilStateCreateInfo depthStencil{ {}, true, true, vk::CompareOp::eLess, false, false };

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{ false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };

        vk::PipelineColorBlendStateCreateInfo colorBlending{ {}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment, { { 0.f, 0.f, 0.f, 0.f } } };

        const auto descriptorSetLayout{ *m_descriptorSetLayout };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ {}, 1, &descriptorSetLayout };
        m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo{ {}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, nullptr, *m_pipelineLayout, *m_renderPass, 0, nullptr, 0 };
        m_graphicsPipeline = m_device->createGraphicsPipelineUnique(nullptr, pipelineInfo);
    }

    void createFramebuffers()
    {
        m_swapChainFramebuffers.clear();

        for (const auto & imageView : m_swapChainImageViews)
        {
            std::array<vk::ImageView, 2> attachments = {
                *imageView,
                *m_depthImageView
            };
            vk::FramebufferCreateInfo framebufferInfo{ {}, *m_renderPass, static_cast<uint32_t>(attachments.size()), attachments.data(), m_swapChainExtent.width, m_swapChainExtent.height, 1 };
            m_swapChainFramebuffers.emplace_back(m_device->createFramebufferUnique(framebufferInfo));
        }
    }

    void createCommandPool()
    {
        const auto queueFamilyIndices = findQueueFamilies(m_physicalDevice);
        vk::CommandPoolCreateInfo poolInfo{ {}, static_cast<uint32_t>(queueFamilyIndices.graphicsFamily) };
        m_commandPool = m_device->createCommandPoolUnique(poolInfo);
    }

    void createDepthResources()
    {
        const auto depthFormat = findDepthFormat();
        createImage(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, m_depthImage, m_depthImageMemory);
        m_depthImageView = createImageView(*m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

        transitionImageLayout(m_depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    }

    vk::Format findSupportedFormat(const std::vector<vk::Format> & candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const
    {
        for (auto format : candidates)
        {
            const auto props{ m_physicalDevice.getFormatProperties(format) };

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }

            if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    vk::Format findDepthFormat() const
    {
        return findSupportedFormat(
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    }

    bool hasStencilComponent(vk::Format format) const
    {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    void createTextureImage()
    {
        int texWidth, texHeight, texChannels;
        auto * pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels)
        {
            throw std::runtime_error("failed to load texture image!");
        }

        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        auto * data = m_device->mapMemory(*stagingBufferMemory, 0, imageSize, {});
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        m_device->unmapMemory(*stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_textureImage, m_textureImageMemory);

        transitionImageLayout(m_textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(m_textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        stagingBuffer.reset(nullptr);
        stagingBufferMemory.reset(nullptr);
    }

    void createTextureImageView()
    {
        m_textureImageView = createImageView(*m_textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
    }

    void createTextureSampler()
    {
        vk::SamplerCreateInfo samplerInfo{ {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.f, true, 16, false, vk::CompareOp::eAlways, 0.f, 0.f, vk::BorderColor::eIntOpaqueBlack, false };
        m_textureSampler = m_device->createSamplerUnique(samplerInfo);
    }

    vk::UniqueImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags) const
    {
        vk::ImageViewCreateInfo viewInfo{ {}, image, vk::ImageViewType::e2D, format, {}, { aspectFlags, 0, 1, 0, 1 } };
        return m_device->createImageViewUnique(viewInfo);
    }

    void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueImage & image, vk::UniqueDeviceMemory & imageMemory)
    {
        vk::ImageCreateInfo imageInfo{ {}, vk::ImageType::e2D, format, { width, height, 1 }, 1, 1, vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive };
        image = m_device->createImageUnique(imageInfo);

        const auto memRequirements = m_device->getImageMemoryRequirements(*image);
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, properties) };
        imageMemory = m_device->allocateMemoryUnique(allocInfo);

        m_device->bindImageMemory(*image, *imageMemory, 0);
    }

    void transitionImageLayout(const vk::UniqueImage & image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const
    {
        auto commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{ {}, {}, oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *image, {} };

        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

            if (hasStencilComponent(format))
            {
                barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        commandBuffer->pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(const vk::UniqueBuffer & buffer, const vk::UniqueImage & image, uint32_t width, uint32_t height) const
    {
        auto commandBuffer = beginSingleTimeCommands();

        vk::BufferImageCopy region{ 0, 0, 0, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, { 0, 0, 0 }, { width, height, 1 } };
        commandBuffer->copyBufferToImage(*buffer, *image, vk::ImageLayout::eTransferDstOptimal, region);

        endSingleTimeCommands(commandBuffer);
    }

    void loadModel()
    {
        Assimp::Importer importer;

        const auto * scene = importer.ReadFile("../models/stanford_dragon/dragon.obj", aiProcess_Triangulate);
        if (!scene)
        {
            throw std::runtime_error(importer.GetErrorString());
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

        const auto meshes = scene->mMeshes;
        const auto numMeshes = scene->mNumMeshes;
        for (uint32_t i = 0; i < numMeshes; ++i)
        {
            const auto mesh = meshes[i];
            const auto vertices0 = mesh->mVertices;
            const auto faces = mesh->mFaces;
            const auto numVertices = mesh->mNumVertices;
            const auto numFaces = mesh->mNumFaces;

            for (uint32_t j = 0; j < numFaces; ++j)
            {
                const auto face = faces[j];
                const auto indices0 = face.mIndices;
                const auto numIndices = face.mNumIndices;
                if (numIndices != 3)
                {
                    throw std::runtime_error("no triangles");
                }

                const auto a_assimp = vertices0[indices0[0]];
                const auto a = glm::vec3(a_assimp.x, a_assimp.y, a_assimp.z);
                const auto b_assimp = vertices0[indices0[1]];
                const auto b = glm::vec3(b_assimp.x, b_assimp.y, b_assimp.z);
                const auto c_assimp = vertices0[indices0[2]];
                const auto c = glm::vec3(c_assimp.x, c_assimp.y, c_assimp.z);
                const auto n = glm::normalize(glm::cross(c - a, b - a));

                for (uint32_t k = 0; k < numIndices; ++k)
                {
                    const auto index = indices0[k];
                    if (index >= numVertices)
                    {
                        throw std::runtime_error("index too big");
                    }

                    const auto v = vertices0[index];

                    Vertex vertex = {};

                    vertex.pos = { v.x, v.y, v.z };
                    vertex.texCoord = { 0.f, 1.f };
                    vertex.color = { 1.0f, 0.0f, 0.0f };
                    vertex.normal = n;

                    if (uniqueVertices.count(vertex) == 0)
                    {
                        uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                        m_vertices.push_back(vertex);
                    }

                    m_indices.push_back(uniqueVertices[vertex]);
                }
            }
        }
    }

    void createVertexBuffer()
    {
        vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        auto * data{ m_device->mapMemory(*stagingBufferMemory, 0, bufferSize, {}) };
        memcpy(data, m_vertices.data(), static_cast<size_t>(bufferSize));
        m_device->unmapMemory(*stagingBufferMemory);

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_vertexBuffer, m_vertexBufferMemory);

        copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

        stagingBuffer.reset(nullptr);
        stagingBufferMemory.reset(nullptr);
    }

    void createIndexBuffer()
    {
        vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        auto * data{ m_device->mapMemory(*stagingBufferMemory, 0, bufferSize, {}) };
        memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
        m_device->unmapMemory(*stagingBufferMemory);

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_indexBuffer, m_indexBufferMemory);

        copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

        stagingBuffer.reset(nullptr);
        stagingBufferMemory.reset(nullptr);
    }

    void createUniformBuffer()
    {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_uniformBuffer, m_uniformBufferMemory);
    }

    void createDescriptorPool()
    {
        std::array<vk::DescriptorPoolSize, 2> poolSizes =
        {
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1 }
        };
        vk::DescriptorPoolCreateInfo poolInfo{ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, static_cast<uint32_t>(poolSizes.size()), poolSizes.data() };
        m_descriptorPool = m_device->createDescriptorPoolUnique(poolInfo);
    }

    void createDescriptorSet()
    {
        vk::DescriptorSetLayout layouts[] = { *m_descriptorSetLayout };
        vk::DescriptorSetAllocateInfo allocInfo{ *m_descriptorPool, 1, layouts };
        m_descriptorSet = std::move(m_device->allocateDescriptorSetsUnique(allocInfo)[0]);

        vk::DescriptorBufferInfo bufferInfo{ *m_uniformBuffer, 0, sizeof(UniformBufferObject) };

        vk::DescriptorImageInfo imageInfo{ *m_textureSampler, *m_textureImageView, vk::ImageLayout::eShaderReadOnlyOptimal };

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites =
        {
            vk::WriteDescriptorSet{ *m_descriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo },
            vk::WriteDescriptorSet{ *m_descriptorSet, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo }
        };

        m_device->updateDescriptorSets(descriptorWrites, nullptr);
    }

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory)
    {
        vk::BufferCreateInfo bufferInfo{ {}, size, usage, vk::SharingMode::eExclusive };
        buffer = m_device->createBufferUnique(bufferInfo);

        const auto memRequirements{ m_device->getBufferMemoryRequirements(*buffer) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, properties) };
        bufferMemory = m_device->allocateMemoryUnique(allocInfo);

        m_device->bindBufferMemory(*buffer, *bufferMemory, 0);
    }

    vk::UniqueCommandBuffer beginSingleTimeCommands() const
    {
        vk::CommandBufferAllocateInfo allocInfo{ *m_commandPool, vk::CommandBufferLevel::ePrimary, 1 };
        auto commandBuffer = m_device->allocateCommandBuffersUnique(allocInfo);

        vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        commandBuffer[0]->begin(beginInfo);

        return std::move(commandBuffer[0]);
    }

    void endSingleTimeCommands(const vk::UniqueCommandBuffer & commandBuffer) const
    {
        commandBuffer->end();

        const auto cmdBuf{ *commandBuffer };
        vk::SubmitInfo submitInfo{ 0, nullptr, nullptr, 1, &cmdBuf };
        m_graphicsQueue.submit(submitInfo, nullptr);
        m_graphicsQueue.waitIdle();
    }

    void copyBuffer(const vk::UniqueBuffer & srcBuffer, const vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const
    {
        auto commandBuffer = beginSingleTimeCommands();

        vk::BufferCopy copyRegion{ 0, 0, size };
        commandBuffer->copyBuffer(*srcBuffer, *dstBuffer, copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        const auto memProperties{ m_physicalDevice.getMemoryProperties() };
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createCommandBuffers()
    {
        vk::CommandBufferAllocateInfo allocInfo{ *m_commandPool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(m_swapChainFramebuffers.size()) };
        m_commandBuffers = m_device->allocateCommandBuffersUnique(allocInfo);

        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            m_commandBuffers[i]->begin(beginInfo);

            std::array<vk::ClearValue, 2> clearValues = {};
            clearValues[0].color = vk::ClearColorValue{ std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } };
            clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
            vk::RenderPassBeginInfo renderPassInfo{ *m_renderPass, *m_swapChainFramebuffers[i], vk::Rect2D{ { 0, 0 }, m_swapChainExtent }, static_cast<uint32_t>(clearValues.size()), clearValues.data() };
            m_commandBuffers[i]->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

            m_commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);

            vk::DeviceSize offsets = 0;
            m_commandBuffers[i]->bindVertexBuffers(0, *m_vertexBuffer, offsets);
            m_commandBuffers[i]->bindIndexBuffer(*m_indexBuffer, 0, vk::IndexType::eUint32);

            m_commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineLayout, 0, *m_descriptorSet, nullptr);

            m_commandBuffers[i]->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

            m_commandBuffers[i]->endRenderPass();

            m_commandBuffers[i]->end();
        }
    }

    void createSemaphores()
    {
        vk::SemaphoreCreateInfo semaphoreInfo;
        m_imageAvailableSemaphore = m_device->createSemaphoreUnique(semaphoreInfo);
        m_renderFinishedSemaphore = m_device->createSemaphoreUnique(semaphoreInfo);
    }

    void updateUniformBuffer() const
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        const auto currentTime = std::chrono::high_resolution_clock::now();
        const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.f;

        UniformBufferObject ubo = {};
        ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
        ubo.view = glm::lookAt(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
        ubo.proj = glm::perspective(glm::radians(45.f), m_swapChainExtent.width / static_cast<float>(m_swapChainExtent.height), 0.1f, 50.f);
        ubo.proj[1][1] *= -1;

        auto * data{ m_device->mapMemory(*m_uniformBufferMemory, 0, sizeof ubo, {}) };
        memcpy(data, &ubo, sizeof ubo);
        m_device->unmapMemory(*m_uniformBufferMemory);
    }

    void drawFrame()
    {
        const auto resultValue{ m_device->acquireNextImageKHR(*m_swapChain, std::numeric_limits<uint64_t>::max(), *m_imageAvailableSemaphore, nullptr) };
        const auto imageIndex{ resultValue.value };
        const auto result{ resultValue.result };

        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapChain();
            return;
        }

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vk::Semaphore waitSemaphores[] = { *m_imageAvailableSemaphore };
        vk::Semaphore signalSemaphores[] = { *m_renderFinishedSemaphore };
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        auto commandBuffer{ *m_commandBuffers[imageIndex] };
        vk::SubmitInfo submitInfo{ 1, waitSemaphores, waitStages, 1, &commandBuffer, 1, signalSemaphores };
        
        m_graphicsQueue.submit(submitInfo, nullptr);

        vk::SwapchainKHR swapChains[] = { *m_swapChain };
        vk::PresentInfoKHR presentInfo{ 1, signalSemaphores, 1, swapChains, &imageIndex };
        const auto presentResult = m_presentQueue.presentKHR(presentInfo);

        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
        {
            recreateSwapChain();
        }
        else if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        m_presentQueue.waitIdle();
    }

    vk::UniqueShaderModule createShaderModule(const std::vector<char> & code) const
    {
        vk::ShaderModuleCreateInfo createInfo{ {}, code.size(), reinterpret_cast<const uint32_t*>(code.data()) };
        return m_device->createShaderModuleUnique(createInfo);
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> & availableFormats) const
    {
        if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
        {
            return{ vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
        }

        for (const auto & availableFormat : availableFormats)
        {
            if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> & availablePresentModes) const
    {
        auto bestMode = vk::PresentModeKHR::eFifo;

        for (const auto & availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox)
            {
                return availablePresentMode;
            }
            
            if (availablePresentMode == vk::PresentModeKHR::eImmediate)
            {
                bestMode = availablePresentMode;
            }
        }

        return bestMode;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR & capabilities) const
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        int width, height;
        glfwGetWindowSize(m_window.get(), &width, &height);

        vk::Extent2D actualExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }

    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice) const
    {
        SwapChainSupportDetails details;
        details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
        details.formats = physicalDevice.getSurfaceFormatsKHR(*m_surface);
        details.presentModes = physicalDevice.getSurfacePresentModesKHR(*m_surface);
        return details;
    }

    bool isDeviceSuitable(vk::PhysicalDevice physicalDevice)
    {
        const auto indices = findQueueFamilies(physicalDevice);
        const auto extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

        auto swapChainAdequate = false;
        if (extensionsSupported)
        {
            const auto swapChainSupport = querySwapChainSupport(physicalDevice);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        const auto supportedFeatures{ physicalDevice.getFeatures() };
        return indices.isComplete() && extensionsSupported && supportedFeatures.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice) const
    {
        const auto availableExtensions{ physicalDevice.enumerateDeviceExtensionProperties() };

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice physicalDevice) const
    {
        QueueFamilyIndices indices;

        const auto queueFamilies{ physicalDevice.getQueueFamilyProperties() };

        auto i = 0;
        for (const auto & queueFamily : queueFamilies)
        {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                indices.graphicsFamily = i;
            }

            const auto presentSupport{ physicalDevice.getSurfaceSupportKHR(i, *m_surface) };

            if (queueFamily.queueCount > 0 && presentSupport)
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            ++i;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions() const
    {
        std::vector<const char*> extensions;

        unsigned int glfwExtensionCount = 0;
        const char ** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        for (unsigned int i = 0; i < glfwExtensionCount; ++i)
        {
            extensions.push_back(glfwExtensions[i]);
        }

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkValidationLayerSupport() const
    {
        const auto availableLayers = vk::enumerateInstanceLayerProperties();
        for (const char * layerName : validationLayers)
        {
            auto layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        const auto fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
    {
        std::string messageString;
        std::string pLayerPrefixString(layerPrefix);

        if (strcmp(layerPrefix, "loader") == 0)
        {
            return VK_FALSE;
        }

        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            messageString += "ERROR: [" + pLayerPrefixString + "] Code " + std::to_string(code) + " : " + msg;
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            messageString += "WARNING: [" + pLayerPrefixString + "] Code " + std::to_string(code) + " : " + msg;
        }
        else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
            messageString += "INFORMATION: [" + pLayerPrefixString + "] Code " + std::to_string(code) + " : " + msg;
        }
        else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            messageString += "DEBUG: [" + pLayerPrefixString + "] Code " + std::to_string(code) + " : " + msg;
        }
        else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            messageString += "PERFORMANCE_WARNING: [" + pLayerPrefixString + "] Code " + std::to_string(code) + " : " + msg;
        }
        else
        {
            return VK_FALSE;
        }

        std::cout << messageString << std::endl;

        return VK_FALSE;
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}