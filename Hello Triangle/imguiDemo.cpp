#include "imguiDemo.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>

#include "shader.hpp"
#include "vulkan_bmvk.hpp"
#include "../ImGuiDemo/imgui_impl_glfw_vulkan.h"
#include <iostream>

namespace bmvk
{
    static void onWindowResized(GLFWwindow * window, int width, int height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        auto app = reinterpret_cast<ImguiDemo *>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    static void onMouseButton(GLFWwindow * window, int button, int action, int mods)
    {
        auto app = reinterpret_cast<ImguiDemo *>(glfwGetWindowUserPointer(window));
        app->imguiMouseButtonCallback(window, button, action, mods);
    }

    static void onScroll(GLFWwindow * window, double xoffset, double yoffset)
    {
        auto app = reinterpret_cast<ImguiDemo *>(glfwGetWindowUserPointer(window));
        app->imguiScrollCallback(window, xoffset, yoffset);
    }

    static void onKey(GLFWwindow * window, int key, int scancode, int action, int mods)
    {
        auto app = reinterpret_cast<ImguiDemo *>(glfwGetWindowUserPointer(window));
        app->imguiKeyCallback(window, key, scancode, action, mods);
    }

    static uint32_t getImguiMemoryType(vk::PhysicalDevice gpu, vk::MemoryPropertyFlags properties, uint32_t type_bits)
    {
        const auto prop{ gpu.getMemoryProperties() };
        for (uint32_t i = 0; i < prop.memoryTypeCount; ++i)
        {
            if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & 1 << i)
            {
                return i;
            }
        }

        return 0xffffffff; // Unable to find memoryType
    }

    ImguiDemo::ImguiDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
        : Demo{ enableValidationLayers, width, height, "Imgui Demo" },
        m_swapchain{ m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window, m_device },
        m_imageAvailableSemaphore{ m_device.createSemaphore() },
        m_renderFinishedSemaphore{ m_device.createSemaphore() },
        m_renderImguiFinishedSemaphore{ m_device.createSemaphore() }
    {
        m_window.setWindowUserPointer(this);
        m_window.setWindowSizeCallback(onWindowResized);
        createRenderPass();
        createFontSampler();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();

        glfwSetMouseButtonCallback(m_window.getPointer().get(), onMouseButton);
        glfwSetScrollCallback(m_window.getPointer().get(), onScroll);
        glfwSetKeyCallback(m_window.getPointer().get(), onKey);

        uploadFonts();
    }

    void ImguiDemo::run()
    {
        while (!m_window.shouldClose())
        {
            m_window.pollEvents();

            // do cpu work here
            updateUniformBuffer();
            imguiNewFrame();

            drawFrame();
        }

        m_device.waitIdle();
    }

    void ImguiDemo::recreateSwapChain()
    {
        m_device.waitIdle();

        for (auto & fb : m_swapChainFramebuffers)
        {
            fb.reset(nullptr);
        }

        m_commandBuffers.clear();
        m_graphicsPipeline.reset(nullptr);
        m_pipelineLayout.reset(nullptr);
        m_renderPass.reset(nullptr);
        m_swapchain.recreate(m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window, m_device);
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }

    void ImguiDemo::createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,{}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        RenderPassCreateInfo renderPassInfo{ {}, colorAttachment, subpass, dependency };
        m_renderPass = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);

        vk::AttachmentDescription colorAttachmentImgui{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR };
        RenderPassCreateInfo renderPassInfoImgui{ {}, colorAttachmentImgui, subpass, dependency };
        m_renderPassImgui = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfoImgui);
    }

    void ImguiDemo::createFontSampler()
    {
        vk::SamplerCreateInfo info{ {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.f, false, 1.f, false, vk::CompareOp::eNever, -1000.f, 1000.f };
        m_fontSamplerImgui = static_cast<vk::Device>(m_device).createSamplerUnique(info);
    }

    void ImguiDemo::createDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
        vk::DescriptorSetLayoutCreateInfo layoutInfo{ {}, 1, &uboLayoutBinding };
        m_descriptorSetLayout = static_cast<vk::Device>(m_device).createDescriptorSetLayoutUnique(layoutInfo);

        vk::Sampler samplers[1] = { *m_fontSamplerImgui };
        vk::DescriptorSetLayoutBinding imageLayoutBinding{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, samplers };
        vk::DescriptorSetLayoutCreateInfo layoutInfoImgui{ {}, 1, &imageLayoutBinding };
        m_descriptorSetLayoutImgui = static_cast<vk::Device>(m_device).createDescriptorSetLayoutUnique(layoutInfoImgui);
    }

    void ImguiDemo::createGraphicsPipeline()
    {
        /*
         * uniformbuffer
         */
        const Shader vertShader{ "../shaders/uniformbuffer.vert.spv", m_device };
        const Shader fragShader{ "../shaders/uniformbuffer.frag.spv", m_device };
        const auto vertShaderStageInfo{ vertShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex) };
        const auto fragShaderStageInfo{ fragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ PipelineVertexInputStateCreateInfo{ bindingDescription, attributeDescriptions } };
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList };
        vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(m_swapchain.getExtent().width), static_cast<float>(m_swapchain.getExtent().height), 0.f, 1.f };
        vk::Rect2D scissor{ {}, m_swapchain.getExtent() };
        vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, &viewport, 1, &scissor };
        vk::PipelineRasterizationStateCreateInfo rasterizer{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, false, 0.f, 0.f, 0.f, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisampling;
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{ false, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlending{ {}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };
        auto descriptorSetLayout{ *m_descriptorSetLayout };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ {}, 1, &descriptorSetLayout };
        m_pipelineLayout = static_cast<vk::Device>(m_device).createPipelineLayoutUnique(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo( {}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending, nullptr, *m_pipelineLayout, *m_renderPass, 0, nullptr, -1 );
        m_graphicsPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, pipelineInfo);

        /*
         * imgui
         */
        static uint32_t glslShaderVertSpv[] =
        {
            0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
            0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
            0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
            0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
            0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
            0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
            0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
            0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
            0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
            0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
            0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
            0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
            0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
            0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
            0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
            0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
            0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
            0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
            0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
            0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
            0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
            0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
            0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
            0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
            0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
            0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
            0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
            0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
            0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
            0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
            0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
            0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
            0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
            0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
            0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
            0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
            0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
            0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
            0x0000002d,0x0000002c,0x000100fd,0x00010038
        };
        vk::ShaderModuleCreateInfo vertModuleInfo{ {}, sizeof glslShaderVertSpv, static_cast<uint32_t*>(glslShaderVertSpv) };
        auto vertModule = static_cast<vk::Device>(m_device).createShaderModuleUnique(vertModuleInfo);
        vk::PipelineShaderStageCreateInfo vertShaderStageInfoImgui{ {}, vk::ShaderStageFlagBits::eVertex, *vertModule, "main" };

        static uint32_t glslShaderFragSpv[] =
        {
            0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
            0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
            0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
            0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
            0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
            0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
            0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
            0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
            0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
            0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
            0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
            0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
            0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
            0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
            0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
            0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
            0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
            0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
            0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
            0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
            0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
            0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
            0x00010038
        };
        vk::ShaderModuleCreateInfo fragModuleInfo{ {}, sizeof glslShaderFragSpv, static_cast<uint32_t*>(glslShaderFragSpv) };
        auto fragModule = static_cast<vk::Device>(m_device).createShaderModuleUnique(fragModuleInfo);
        vk::PipelineShaderStageCreateInfo fragShaderStageInfoImgui{ {}, vk::ShaderStageFlagBits::eFragment, *fragModule, "main" };

        vk::PipelineShaderStageCreateInfo shaderStagesImgui[] = { vertShaderStageInfoImgui, fragShaderStageInfoImgui };

        vk::VertexInputBindingDescription bindingDescriptionImgui{ 0, sizeof ImDrawVert, vk::VertexInputRate::eVertex };
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptionsImgui
        {
            { 0, 0, vk::Format::eR32G32Sfloat, reinterpret_cast<size_t>(&static_cast<ImDrawVert*>(nullptr)->pos) },
            { 1, 0, vk::Format::eR32G32Sfloat, reinterpret_cast<size_t>(&static_cast<ImDrawVert*>(nullptr)->uv) },
            { 2, 0, vk::Format::eR8G8B8A8Unorm, reinterpret_cast<size_t>(&static_cast<ImDrawVert*>(nullptr)->col) }
        };
        vk::PipelineVertexInputStateCreateInfo vertexInputInfoImgui{ PipelineVertexInputStateCreateInfo{ bindingDescriptionImgui, attributeDescriptionsImgui } };
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyImgui{ PipelineInputAssemblyStateCreateInfo{ vk::PrimitiveTopology::eTriangleList } };
        vk::PipelineViewportStateCreateInfo viewportStateImgui{ PipelineViewportStateCreateInfo{ viewport, scissor } };
        vk::PipelineRasterizationStateCreateInfo rasterizerImgui{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, false, false, false, false, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisamplingImgui;
        vk::PipelineDepthStencilStateCreateInfo depthStencilState;
        vk::PipelineColorBlendAttachmentState colorBlendAttachmentImgui{ true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlendingImgui{ PipelineColorBlendStateCreateInfo{ false, vk::LogicOp::eClear, colorBlendAttachmentImgui } };
        vk::PipelineDynamicStateCreateInfo dynamicState{ PipelineDynamicStateCreateInfo{ { vk::DynamicState::eViewport, vk::DynamicState::eScissor } } };

        vk::PushConstantRange pushConstantRange{ vk::ShaderStageFlagBits::eVertex, sizeof(float) * 0, sizeof(float) * 4 };
        auto descriptorSetLayoutImgui{ *m_descriptorSetLayoutImgui };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfoImgui{ PipelineLayoutCreateInfo{ descriptorSetLayoutImgui, pushConstantRange } };
        m_pipelineLayoutImgui = static_cast<vk::Device>(m_device).createPipelineLayoutUnique(pipelineLayoutInfoImgui);

        vk::GraphicsPipelineCreateInfo pipelineInfoImgui{ {}, 2, shaderStagesImgui, &vertexInputInfoImgui, &inputAssemblyImgui, nullptr, &viewportStateImgui, &rasterizerImgui, &multisamplingImgui, &depthStencilState, &colorBlendingImgui, &dynamicState, *m_pipelineLayoutImgui, *m_renderPassImgui, 0, nullptr, -1 };
        m_graphicsPipelineImgui = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, pipelineInfoImgui);
    }

    void ImguiDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.clear();
        for (auto & uniqueImageView : m_swapchain.getImageViews())
        {
            auto imageView{ *uniqueImageView };
            m_swapChainFramebuffers.emplace_back(m_device.createFramebuffer(m_renderPass, imageView, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1));
        }
    }

    void ImguiDemo::createVertexBuffer()
    {
        const auto bufferSize{ sizeof(vertices[0]) * vertices.size() };

        const auto stagingBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferSrc };
        const auto stagingBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, stagingBufferUsageFlags, stagingBufferMemoryPropertyFlags, stagingBuffer, stagingBufferMemory);

        auto data{ static_cast<vk::Device>(m_device).mapMemory(*stagingBufferMemory, 0, bufferSize) };
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        static_cast<vk::Device>(m_device).unmapMemory(*stagingBufferMemory);

        const auto vertexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer };
        const auto vertexBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eDeviceLocal };
        createBuffer(bufferSize, vertexBufferUsageFlags, vertexBufferMemoryPropertyFlags, m_vertexBuffer, m_vertexBufferMemory);

        copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
    }

    void ImguiDemo::createIndexBuffer()
    {
        const auto bufferSize{ sizeof(indices[0]) * indices.size() };

        const auto stagingBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferSrc };
        const auto stagingBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, stagingBufferUsageFlags, stagingBufferMemoryPropertyFlags, stagingBuffer, stagingBufferMemory);

        auto data{ static_cast<vk::Device>(m_device).mapMemory(*stagingBufferMemory, 0, bufferSize) };
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        static_cast<vk::Device>(m_device).unmapMemory(*stagingBufferMemory);

        const auto indexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer };
        const auto indexBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eDeviceLocal };
        createBuffer(bufferSize, indexBufferUsageFlags, indexBufferMemoryPropertyFlags, m_indexBuffer, m_indexBufferMemory);

        copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
    }

    void ImguiDemo::createUniformBuffer()
    {
        const auto bufferSize{ sizeof(UniformBufferObject) };
        const auto uniformBufferUsageFlags{ vk::BufferUsageFlagBits::eUniformBuffer };
        const auto uniformBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        createBuffer(bufferSize, uniformBufferUsageFlags, uniformBufferMemoryPropertyFlags, m_uniformBuffer, m_uniformBufferMemory);
    }

    void ImguiDemo::createDescriptorPool()
    {
        vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
        m_descriptorPool = m_device.createDescriptorPool(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSize);

        vk::DescriptorPoolSize poolSizeImgui[11] =
        {
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 }
        };
        m_descriptorPoolImgui = m_device.createDescriptorPool(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000 * 11, vk::ArrayProxy<vk::DescriptorPoolSize>(11, poolSizeImgui));
    }

    void ImguiDemo::createDescriptorSet()
    {
        vk::DescriptorSetLayout layouts[] = { *m_descriptorSetLayout };
        vk::DescriptorSetAllocateInfo allocInfo{ *m_descriptorPool, 1, layouts };
        m_descriptorSets = static_cast<vk::Device>(m_device).allocateDescriptorSetsUnique(allocInfo);

        vk::DescriptorBufferInfo bufferInfo{ *m_uniformBuffer, 0, sizeof(UniformBufferObject) };
        WriteDescriptorSet descriptorWrite{ m_descriptorSets[0], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo };
        m_device.updateDescriptorSet(descriptorWrite);

        vk::DescriptorSetLayout layoutsImgui[] = { *m_descriptorSetLayoutImgui };
        vk::DescriptorSetAllocateInfo allocInfoImgui{ *m_descriptorPoolImgui, 1, layoutsImgui };
        m_descriptorSetsImgui = static_cast<vk::Device>(m_device).allocateDescriptorSetsUnique(allocInfoImgui);
    }

    void ImguiDemo::createCommandBuffers()
    {
        m_commandBuffers = m_device.allocateCommandBuffers(m_commandPool, static_cast<uint32_t>(m_swapChainFramebuffers.size()));
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            const auto & cmdBuffer{ m_commandBuffers[i] };
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            cmdBuffer.beginRenderPass(m_renderPass, m_swapChainFramebuffers[i], { { 0, 0 }, m_swapchain.getExtent() });
            cmdBuffer.bindPipeline(m_graphicsPipeline);
            cmdBuffer.bindDescriptorSet(m_pipelineLayout, m_descriptorSets[0]);
            cmdBuffer.bindVertexBuffer(m_vertexBuffer);
            cmdBuffer.bindIndexBuffer(m_indexBuffer);
            cmdBuffer.drawIndexed(static_cast<uint32_t>(indices.size()));
            cmdBuffer.endRenderPass();
            cmdBuffer.end();
        }
    }

    void ImguiDemo::uploadFonts()
    {
        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        auto & io = ImGui::GetIO();

        unsigned char * pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        auto upload_size = width * height * 4 * sizeof(char);

        // Create the Image:
        vk::ImageCreateInfo imageInfo{ {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(width, height, 1), 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst };
        m_imguiFontImage = static_cast<vk::Device>(m_device).createImageUnique(imageInfo);

        const auto imageReq{ static_cast<vk::Device>(m_device).getImageMemoryRequirements(*m_imguiFontImage) };
        const auto imageMemoryTypeIndex{ getImguiMemoryType(static_cast<vk::PhysicalDevice>(m_instance.getPhysicalDevice()), vk::MemoryPropertyFlagBits::eDeviceLocal, imageReq.memoryTypeBits) };
        vk::MemoryAllocateInfo imageMemoryAllocInfo{ imageReq.size, imageMemoryTypeIndex };
        m_imguiFontMemory = static_cast<vk::Device>(m_device).allocateMemoryUnique(imageMemoryAllocInfo);

        static_cast<vk::Device>(m_device).bindImageMemory(*m_imguiFontImage, *m_imguiFontMemory, 0);

        // Create the Image View:
        vk::ImageViewCreateInfo imageViewInfo{ {}, *m_imguiFontImage, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm,{}, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
        m_imguiFontImageView = static_cast<vk::Device>(m_device).createImageViewUnique(imageViewInfo);

        // Update the Descriptor Set:
        vk::DescriptorImageInfo descImage[1] = { vk::DescriptorImageInfo{ *m_fontSamplerImgui, *m_imguiFontImageView, vk::ImageLayout::eShaderReadOnlyOptimal } };
        vk::WriteDescriptorSet writeDesc(*m_descriptorSetsImgui[0], 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, descImage);
        static_cast<vk::Device>(m_device).updateDescriptorSets(writeDesc, nullptr);

        // Create the Upload Buffer:
        vk::BufferCreateInfo bufferInfo{ {}, upload_size, vk::BufferUsageFlagBits::eTransferSrc };
        auto uploadBuffer{ static_cast<vk::Device>(m_device).createBufferUnique(bufferInfo) };

        const auto bufferReq{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*uploadBuffer) };
        m_imguiBufferMemoryAlignment = m_imguiBufferMemoryAlignment > bufferReq.alignment ? m_imguiBufferMemoryAlignment : bufferReq.alignment;
        const auto bufferMemoryTypeIndex{ getImguiMemoryType(static_cast<vk::PhysicalDevice>(m_instance.getPhysicalDevice()), vk::MemoryPropertyFlagBits::eHostVisible, bufferReq.memoryTypeBits) };
        vk::MemoryAllocateInfo bufferMemoryAllocInfo{ bufferReq.size, bufferMemoryTypeIndex };
        auto uploadBufferMemory{ static_cast<vk::Device>(m_device).allocateMemoryUnique(bufferMemoryAllocInfo) };

        static_cast<vk::Device>(m_device).bindBufferMemory(*uploadBuffer, *uploadBufferMemory, 0);

        // Upload to Buffer:
        auto * map{ m_device.mapMemory(uploadBufferMemory, upload_size) };
        memcpy(map, pixels, upload_size);
        vk::MappedMemoryRange flushRange{ *uploadBufferMemory, 0, upload_size };
        static_cast<vk::Device>(m_device).flushMappedMemoryRanges(flushRange);
        m_device.unmapMemory(uploadBufferMemory);

        // Copy to Image:
        vk::ImageMemoryBarrier copyBarrier{ {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *m_imguiFontImage, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
        cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, copyBarrier);

        vk::BufferImageCopy copyRegion{ 0, 0, 0, vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },{}, vk::Extent3D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 } };
        cmdBuffer.copyBufferToImage(uploadBuffer, m_imguiFontImage, vk::ImageLayout::eTransferDstOptimal, copyRegion);

        vk::ImageMemoryBarrier useBarrier{ vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *m_imguiFontImage, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
        cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, useBarrier);

        // Store our identifier
        VkImage img{ *m_imguiFontImage };
        io.Fonts->TexID = reinterpret_cast<void *>((intptr_t)img);

        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_device.waitIdle();
    }

    void ImguiDemo::drawFrame()
    {
        m_queue.waitIdle();
        timing();

        uint32_t imageIndex;
        try
        {
            imageIndex = m_device.acquireNextImage(m_swapchain, m_imageAvailableSemaphore);
        }
        catch (const vk::OutOfDateKHRError &)
        {
            recreateSwapChain();
            return;
        }

        m_queue.submit(m_commandBuffers[imageIndex], m_imageAvailableSemaphore, m_renderFinishedSemaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
        drawImgui(imageIndex);

        auto waitSemaphore{ *m_renderImguiFinishedSemaphore };
        auto swapchain{ static_cast<vk::SwapchainKHR>(m_swapchain) };
        auto success{ m_queue.present(waitSemaphore, swapchain, imageIndex) };
        if (!success)
        {
            recreateSwapChain();
        }
    }

    void ImguiDemo::drawImgui(uint32_t imageIndex)
    {
        m_commandBufferImguiPtr.reset();
        m_commandBufferImguiPtr = std::make_unique<CommandBuffer>(m_device.allocateCommandBuffer(m_commandPool));
        m_commandBufferImguiPtr->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        m_commandBufferImguiPtr->beginRenderPass(m_renderPassImgui, m_swapChainFramebuffers[imageIndex], { { 0, 0 }, m_swapchain.getExtent() });

        ImGui::Render();

        m_commandBufferImguiPtr->endRenderPass();
        m_commandBufferImguiPtr->end();

        m_queue.submit(*m_commandBufferImguiPtr.get(), m_renderFinishedSemaphore, m_renderImguiFinishedSemaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
    }

    void ImguiDemo::updateUniformBuffer()
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.f;

        UniformBufferObject ubo;
        ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.f), m_swapchain.getExtent().width / static_cast<float>(m_swapchain.getExtent().height), 0.1f, 10.f);
        ubo.proj[1][1] *= -1;

        m_device.copyToMemory(m_uniformBufferMemory, ubo);
    }

    void ImguiDemo::imguiNewFrame()
    {
        const auto & windowPtr{ m_window.getPointer() };
        auto & io{ ImGui::GetIO() };

        // Setup display size (every frame to accommodate for window resizing)
        int w, h;
        int display_w, display_h;
        glfwGetWindowSize(windowPtr.get(), &w, &h);
        glfwGetFramebufferSize(windowPtr.get(), &display_w, &display_h);
        io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
        io.DisplayFramebufferScale = ImVec2(w > 0 ? static_cast<float>(display_w) / w : 0, h > 0 ? static_cast<float>(display_h) / h : 0);

        // Setup time step
        const auto current_time = glfwGetTime();
        io.DeltaTime = m_imguiTime > 0.0 ? static_cast<float>(current_time - m_imguiTime) : static_cast<float>(1.0f / 60.0f);
        m_imguiTime = current_time;

        // Setup inputs
        // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
        if (glfwGetWindowAttrib(windowPtr.get(), GLFW_FOCUSED))
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(windowPtr.get(), &mouse_x, &mouse_y);
            io.MousePos = ImVec2(static_cast<float>(mouse_x), static_cast<float>(mouse_y));   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
        }
        else
        {
            io.MousePos = ImVec2(-1, -1);
        }

        for (auto i = 0; i < 3; i++)
        {
            io.MouseDown[i] = m_imguiMousePressed[i] || glfwGetMouseButton(windowPtr.get(), i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            m_imguiMousePressed[i] = false;
        }

        io.MouseWheel = m_imguiMouseWheel;
        m_imguiMouseWheel = 0.0f;

        // Hide OS mouse cursor if ImGui is drawing it
        glfwSetInputMode(windowPtr.get(), GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

        // Start the frame
        ImGui::NewFrame();
    }

    void ImguiDemo::imguiMouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
    {
        if (action == GLFW_PRESS && button >= 0 && button < 3)
        {
            m_imguiMousePressed[button] = true;
        }
    }

    void ImguiDemo::imguiScrollCallback(GLFWwindow * window, double xoffset, double yoffset)
    {
        m_imguiMouseWheel += static_cast<float>(yoffset); // Use fractional mouse wheel, 1.0 unit 5 lines.
    }

    void ImguiDemo::imguiKeyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) const
    {
        auto & io{ ImGui::GetIO() };
        if (action == GLFW_PRESS)
        {
            io.KeysDown[key] = true;
        }

        if (action == GLFW_RELEASE)
        {
            io.KeysDown[key] = false;
        }

        (void)mods; // Modifiers are not reliable across systems
        io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
        io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    }
}
