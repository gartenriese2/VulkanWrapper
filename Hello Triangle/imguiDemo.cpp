#include "imguiDemo.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp"
#include "vulkan_bmvk.hpp"
#include "../Imgui/imgui.h"

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

    ImguiDemo::ImguiDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
        : Demo{ enableValidationLayers, width, height, "Imgui Demo" },
        m_swapchain{ m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window, m_device },
        m_imageAvailableSemaphore{ m_device.createSemaphore() },
        m_renderFinishedSemaphore{ m_device.createSemaphore() }
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
    }

    void ImguiDemo::run()
    {
        while (!m_window.shouldClose())
        {
            m_window.pollEvents();

            // do cpu work here
            updateUniformBuffer();

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
        vk::AttachmentDescription colorAttachment{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,{}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        RenderPassCreateInfo renderPassInfo{ {}, colorAttachment, subpass, dependency };
        m_renderPass = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);
        m_renderPassImgui = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);
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

        m_commandBuffersImgui = m_device.allocateCommandBuffers(m_commandPool, static_cast<uint32_t>(m_swapChainFramebuffers.size()));
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

        auto waitSemaphore{ *m_renderFinishedSemaphore };
        auto swapchain{ static_cast<vk::SwapchainKHR>(m_swapchain) };
        auto success{ m_queue.present(waitSemaphore, swapchain, imageIndex) };
        if (!success)
        {
            recreateSwapChain();
        }
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
}
