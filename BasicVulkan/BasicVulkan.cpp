#include <vw/window.hpp>
#include <vw/camera.hpp>

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

constexpr int32_t K_WIDTH{ 800 };
constexpr int32_t K_HEIGHT{ 600 };

const std::string K_MODEL_PATH{ "../models/stanford_dragon/dragon.obj" };
const std::string K_TEXTURE_PATH{ "../textures/chalet.jpg" };
const std::string K_VERTEX_SHADER_PATH{ "../shaders/blinnphong.vert.spv" };
const std::string K_FRAGMENT_SHADER_PATH{ "../shaders/blinnphong.frag.spv" };

const std::vector<const char*> K_VALIDATION_LAYERS = {
    "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> K_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
constexpr bool enableValidationLayers{ false };
#else
constexpr bool enableValidationLayers{ true };
#endif

constexpr bool enableCompleteDebugOutput{ true };

struct QueueFamilyIndices
{
    int32_t graphicsFamily = -1;
    int32_t presentFamily = -1;

    auto isComplete() const
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
        return { 0, sizeof Vertex, vk::VertexInputRate::eVertex };
    }

    static auto getAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions =
        {
            vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) },
            vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) },
            vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) },
            vk::VertexInputAttributeDescription{ 3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) }
        };
        return attributeDescriptions;
    }

    auto operator==(const Vertex & other) const
    {
        return pos == other.pos && normal == other.normal && color == other.color && texCoord == other.texCoord;
    }
};

struct Model
{
    void translate(const glm::vec3 & translate)
    {
        modelMatrix = glm::translate(modelMatrix, translate);
    }
    
    void scale(const glm::vec3 & scale)
    {
        modelMatrix = glm::scale(modelMatrix, scale);
    }

    void rotate(const glm::vec3 & axis, const float radians)
    {
        modelMatrix = glm::rotate(modelMatrix, radians, axis);
    }

    void pushConstants(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniquePipelineLayout & pipelineLayout) const
    {
        std::array<glm::mat4, 1> pushConstants = { modelMatrix };
        commandBuffer->pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof pushConstants, pushConstants.data());
    }

    void draw(const vk::UniqueCommandBuffer & commandBuffer) const
    {
        vk::DeviceSize offsets = 0;
        commandBuffer->bindVertexBuffers(0, *vertexBuffer, offsets);
        commandBuffer->bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffer->drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    void reset()
    {
        indexBuffer.reset(nullptr);
        indexBufferMemory.reset(nullptr);
        vertexBuffer.reset(nullptr);
        vertexBufferMemory.reset(nullptr);
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vk::UniqueBuffer vertexBuffer;
    vk::UniqueDeviceMemory vertexBufferMemory;
    vk::UniqueBuffer indexBuffer;
    vk::UniqueDeviceMemory indexBufferMemory;

    glm::mat4 modelMatrix;
};

namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const & vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1) ^ hash<glm::vec3>()(vertex.normal);
        }
    };
}

struct UniformBufferObject
{
    glm::mat4 view;
    glm::mat4 proj;
};

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow(K_WIDTH, K_HEIGHT, "Blinn-Phong Demo");
        initVulkan();
        mainLoop();
        cleanup();
    }
private:
    std::unique_ptr<vw::util::Window> m_window;
    vw::util::Camera m_camera;

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

    Model m_dragonModel;
    Model m_cornellModel;

    vk::UniqueBuffer m_uniformBuffer;
    vk::UniqueDeviceMemory m_uniformBufferMemory;

    vk::UniqueDescriptorPool m_descriptorPool;
    vk::UniqueDescriptorSet m_descriptorSet;

    std::vector<vk::UniqueCommandBuffer> m_commandBuffers;

    vk::UniqueSemaphore m_imageAvailableSemaphore;
    vk::UniqueSemaphore m_renderFinishedSemaphore;

    void initWindow(const int32_t width, const int32_t height, std::string_view title)
    {
        m_window.reset(new vw::util::Window(width, height, title.data()));
        m_window->setWindowUserPointer(this);
        m_window->setWindowSizeCallback(onWindowResized);

        m_camera = vw::util::Camera(glm::vec3{ -10.f, 40.f, -80.f }, glm::vec3{ -1.f, -1.f, 0.15f }, glm::vec3{ 0.f, -1.f, 0.f }, 45.f, 1.f, 0.01f, std::numeric_limits<float>::infinity());
        m_camera.rotate(glm::radians(180.f), glm::vec3{ 0.f, 1.f, 0.f });

        m_window->setKeyCallback([](GLFWwindow * window, int key, int scancode, int action, int mods)
        {
            auto * app = reinterpret_cast<HelloTriangleApplication *>(glfwGetWindowUserPointer(window));

            if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
            {
                if ((mods & GLFW_MOD_SHIFT) != 0)
                {
                    app->m_camera.translateLocal({ 0.f, 0.f, -2.5f });
                }
                else
                {
                    app->m_camera.translateLocal({ 0.f, 0.f, -0.5f });
                }
            }

            if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
            {
                if ((mods & GLFW_MOD_SHIFT) != 0)
                {
                    app->m_camera.translateLocal({ 0.f, 0.f, 2.5f });
                }
                else
                {
                    app->m_camera.translateLocal({ 0.f, 0.f, 0.5f });
                }
            }

            if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
            {
                if ((mods & GLFW_MOD_SHIFT) != 0)
                {
                    app->m_camera.translateLocal({ -2.5f, 0.f, 0.f });
                }
                else
                {
                    app->m_camera.translateLocal({ -0.5f, 0.f, 0.f });
                }
            }

            if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
            {
                if ((mods & GLFW_MOD_SHIFT) != 0)
                {
                    app->m_camera.translateLocal({ 2.5f, 0.f, 0.f });
                }
                else
                {
                    app->m_camera.translateLocal({ 0.5f, 0.f, 0.f });
                }
            }

            if (key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT))
            {
                app->m_camera.rotate(glm::radians(5.f), glm::vec3{ 0.f, 1.f, 0.f });
            }

            if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT))
            {
                app->m_camera.rotate(-glm::radians(5.f), glm::vec3{ 0.f, 1.f, 0.f });
            }

            if (key == GLFW_KEY_P && (action == GLFW_PRESS || action == GLFW_REPEAT))
            {
                const auto & pos{ app->m_camera.getPos() };
                const auto & dir{ app->m_camera.getDir() };
                const auto & up{ app->m_camera.getUp() };
                std::cout << "Pos: (" << pos.x << '|' << pos.y << '|' << pos.z << ")\n";
                std::cout << "Dir: (" << dir.x << '|' << dir.y << '|' << dir.z << ")\n";
                std::cout << "Up: (" << up.x << '|' << up.y << '|' << up.z << ")\n";
            }

            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            {
                app->m_window->setShouldClose(true);
            }
        });
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
        loadModels();
        createVertexBuffer(m_dragonModel);
        createIndexBuffer(m_dragonModel);
        createVertexBuffer(m_cornellModel);
        createIndexBuffer(m_cornellModel);
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
        createSemaphores();
    }

    void mainLoop()
    {
        while (!m_window->shouldClose())
        {
            m_window->pollEvents();

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

        m_dragonModel.reset();
        m_cornellModel.reset();

        m_renderFinishedSemaphore.reset(nullptr);
        m_imageAvailableSemaphore.reset(nullptr);

        m_commandPool.reset(nullptr);

        m_device.reset(nullptr);
        m_callback.reset(nullptr);
        m_surface.reset(nullptr);
        m_instance.reset(nullptr);

        m_window.reset(nullptr);
    }

    static void onWindowResized(GLFWwindow* window, int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        auto * app = reinterpret_cast<HelloTriangleApplication *>(glfwGetWindowUserPointer(window));
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
        std::vector<const char*> extensionsAsCstrings{};
        for (const auto & string : extensions)
        {
            extensionsAsCstrings.emplace_back(string.c_str());
        }

        vk::InstanceCreateInfo createInfo{ {}, &appInfo, 0, nullptr, static_cast<uint32_t>(extensionsAsCstrings.size()), extensionsAsCstrings.data() };
        if (enableValidationLayers) {
            createInfo.setEnabledLayerCount(static_cast<uint32_t>(K_VALIDATION_LAYERS.size()));
            createInfo.setPpEnabledLayerNames(K_VALIDATION_LAYERS.data());
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
        m_surface = m_window->createSurface(m_instance);
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

        vk::DeviceCreateInfo createInfo{ {}, static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(), 0, nullptr, static_cast<uint32_t>(K_DEVICE_EXTENSIONS.size()), K_DEVICE_EXTENSIONS.data(), &deviceFeatures };
        if (enableValidationLayers)
        {
            createInfo.setEnabledLayerCount(static_cast<uint32_t>(K_VALIDATION_LAYERS.size()));
            createInfo.setPpEnabledLayerNames(K_VALIDATION_LAYERS.data());
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
        auto vertShaderCode = readFile(K_VERTEX_SHADER_PATH);
        auto fragShaderCode = readFile(K_FRAGMENT_SHADER_PATH);

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

        vk::PushConstantRange pushConstant{ vk::ShaderStageFlagBits::eVertex, 0, sizeof glm::mat4 };
        const auto descriptorSetLayout{ *m_descriptorSetLayout };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ {}, 1, &descriptorSetLayout, 1, &pushConstant };
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
        auto * pixels = stbi_load(K_TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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

    void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueImage & image, vk::UniqueDeviceMemory & imageMemory) const
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

    void loadModel(Assimp::Importer & importer, Model & model, std::string_view file) const
    {
        model.vertices.clear();
        model.indices.clear();

        const auto * scene = importer.ReadFile(file.data(), aiProcess_Triangulate);
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
                        uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
                        model.vertices.push_back(vertex);
                    }

                    model.indices.push_back(uniqueVertices[vertex]);
                }
            }
        }
    }

    void loadModels()
    {
        Assimp::Importer importer;

        loadModel(importer, m_cornellModel, "../models/cornell_box/cornell_box.obj");
        m_cornellModel.scale(glm::vec3{ 0.1f });
        m_cornellModel.translate(glm::vec3{ -400.f, 0.f, -60.f });
        loadModel(importer, m_dragonModel, "../models/stanford_dragon/dragon.obj");
        m_dragonModel.scale(glm::vec3{ 2.f });
    }

    void createVertexBuffer(Model & model) const
    {
        vk::DeviceSize bufferSize = sizeof(model.vertices[0]) * model.vertices.size();

        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        auto * data{ m_device->mapMemory(*stagingBufferMemory, 0, bufferSize, {}) };
        memcpy(data, model.vertices.data(), static_cast<size_t>(bufferSize));
        m_device->unmapMemory(*stagingBufferMemory);

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, model.vertexBuffer, model.vertexBufferMemory);

        copyBuffer(stagingBuffer, model.vertexBuffer, bufferSize);

        stagingBuffer.reset(nullptr);
        stagingBufferMemory.reset(nullptr);
    }

    void createIndexBuffer(Model & model) const
    {
        vk::DeviceSize bufferSize = sizeof(model.indices[0]) * model.indices.size();

        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        auto * data{ m_device->mapMemory(*stagingBufferMemory, 0, bufferSize, {}) };
        memcpy(data, model.indices.data(), static_cast<size_t>(bufferSize));
        m_device->unmapMemory(*stagingBufferMemory);

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, model.indexBuffer, model.indexBufferMemory);

        copyBuffer(stagingBuffer, model.indexBuffer, bufferSize);

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

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory) const
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
            m_commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineLayout, 0, *m_descriptorSet, nullptr);

            m_dragonModel.pushConstants(m_commandBuffers[i], m_pipelineLayout);
            m_dragonModel.draw(m_commandBuffers[i]);

            m_cornellModel.pushConstants(m_commandBuffers[i], m_pipelineLayout);
            m_cornellModel.draw(m_commandBuffers[i]);

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

        UniformBufferObject ubo;
        ubo.view = m_camera.getViewMatrix();
        ubo.proj = m_camera.getProjMatrix();
        /*ubo.view = glm::lookAt(glm::vec3(0.f, 40.f, -80.f), glm::vec3(0.f, 20.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
        ubo.proj = glm::perspective(glm::radians(45.f), m_swapChainExtent.width / static_cast<float>(m_swapChainExtent.height), 0.1f, 200.f);*/
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
        std::tie(width, height) = m_window->getSize();

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

    bool isDeviceSuitable(vk::PhysicalDevice physicalDevice) const
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

        std::set<std::string> requiredExtensions(K_DEVICE_EXTENSIONS.begin(), K_DEVICE_EXTENSIONS.end());

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

    std::vector<std::string> getRequiredExtensions() const
    {
        std::vector<std::string> extensions;

        const auto glfwExtensions{ m_window->getRequiredExtensions() };

        for (unsigned int i = 0; i < glfwExtensions.size(); ++i)
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
        for (const char * layerName : K_VALIDATION_LAYERS)
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