#include "imguiBaseDemo.hpp"

#include <imgui/imgui.h>

#include "shader.hpp"
#include "vulkan_bmvk.hpp"

namespace bmvk
{
    static void onWindowResized(GLFWwindow * window, int width, int height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        auto app = reinterpret_cast<ImguiBaseDemo *>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    static void onMouseButton(GLFWwindow * window, int button, int action, int mods)
    {
        auto app = reinterpret_cast<ImguiBaseDemo *>(glfwGetWindowUserPointer(window));
        app->imguiMouseButtonCallback(window, button, action, mods);
    }

    static void onScroll(GLFWwindow * window, double xoffset, double yoffset)
    {
        auto app = reinterpret_cast<ImguiBaseDemo *>(glfwGetWindowUserPointer(window));
        app->imguiScrollCallback(window, xoffset, yoffset);
    }

    static void onKey(GLFWwindow * window, int key, int scancode, int action, int mods)
    {
        auto app = reinterpret_cast<ImguiBaseDemo *>(glfwGetWindowUserPointer(window));
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

    ImguiBaseDemo::ImguiBaseDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name, const bool onlyWarningsAndAbove)
      : Demo{ enableValidationLayers, width, height, name, onlyWarningsAndAbove },
        m_swapchain{ m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window.getSize(), m_device },
        m_fontSampler{ m_device.createSampler(false, -1000.f, 1000.f) }
    {
        m_window.setWindowUserPointer(this);
        m_window.setWindowSizeCallback(onWindowResized);
        
        createDescriptorSetLayout();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createDescriptorPool();
        createDescriptorSet();

        m_window.setMouseButtonCallback(onMouseButton);
        m_window.setScrollCallback(onScroll);
        m_window.setKeyCallback(onKey);

        uploadFonts();

        auto & io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
        io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
        io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
        io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
        io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
        io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
        io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
        io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
        io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
        io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
        io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
        io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
        io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
        io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
        io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
        io.RenderDrawListsFn = nullptr;
    }

    void ImguiBaseDemo::recreateSwapChain()
    {
        m_graphicsPipelineImgui.reset(nullptr);
        m_pipelineLayoutImgui.reset(nullptr);
        m_renderPassImgui.reset(nullptr);

        m_swapchain.recreate(m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window.getSize(), m_device);

        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
    }

    void ImguiBaseDemo::createDescriptorSetLayout()
    {
        auto & sampler{ reinterpret_cast<vk::UniqueSampler &>(m_fontSampler) };
        vk::Sampler samplers[1] = { *sampler };
        vk::DescriptorSetLayoutBinding imageLayoutBinding{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, samplers };
        vk::DescriptorSetLayoutCreateInfo layoutInfoImgui{ {}, 1, &imageLayoutBinding };
        m_descriptorSetLayoutImgui = static_cast<vk::Device>(m_device).createDescriptorSetLayoutUnique(layoutInfoImgui);
    }

    void ImguiBaseDemo::createRenderPass()
    {
        vk::AttachmentDescription colorAttachmentImgui{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,{}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        RenderPassCreateInfo renderPassInfoImgui{ {}, colorAttachmentImgui, subpass, dependency };
        m_renderPassImgui = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfoImgui);
    }

    void ImguiBaseDemo::createGraphicsPipeline()
    {
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
        vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(m_swapchain.getExtent().width), static_cast<float>(m_swapchain.getExtent().height), 0.f, 1.f };
        vk::Rect2D scissor{ {}, m_swapchain.getExtent() };
        vk::PipelineViewportStateCreateInfo viewportStateImgui{ PipelineViewportStateCreateInfo{ viewport, scissor } };
        vk::PipelineRasterizationStateCreateInfo rasterizerImgui{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, false, false, false, false, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisamplingImgui;
        vk::PipelineDepthStencilStateCreateInfo depthStencilState;
        vk::PipelineColorBlendAttachmentState colorBlendAttachmentImgui{ true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlendingImgui{ PipelineColorBlendStateCreateInfo{ false, vk::LogicOp::eClear, colorBlendAttachmentImgui } };
        vk::PipelineDynamicStateCreateInfo dynamicState{ PipelineDynamicStateCreateInfo{ { vk::DynamicState::eViewport, vk::DynamicState::eScissor } } };

        vk::PushConstantRange pushConstantRange{ vk::ShaderStageFlagBits::eVertex, sizeof(float) * 0, sizeof(float) * 4 };
        m_pipelineLayoutImgui = m_device.createPipelineLayout({ *m_descriptorSetLayoutImgui }, { pushConstantRange });

        vk::GraphicsPipelineCreateInfo pipelineInfoImgui{ {}, 2, shaderStagesImgui, &vertexInputInfoImgui, &inputAssemblyImgui, nullptr, &viewportStateImgui, &rasterizerImgui, &multisamplingImgui, &depthStencilState, &colorBlendingImgui, &dynamicState, *m_pipelineLayoutImgui, *m_renderPassImgui, 0, nullptr, -1 };
        m_graphicsPipelineImgui = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, pipelineInfoImgui);
    }

    void ImguiBaseDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.clear();
        for (auto & uniqueImageView : m_swapchain.getImageViews())
        {
            auto imageView{ *uniqueImageView };
            m_swapChainFramebuffers.emplace_back(m_device.createFramebuffer(m_renderPassImgui, imageView, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1));
        }
    }

    void ImguiBaseDemo::createDescriptorPool()
    {
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

    void ImguiBaseDemo::createDescriptorSet()
    {
        vk::DescriptorSetLayout layoutsImgui[] = { *m_descriptorSetLayoutImgui };
        vk::DescriptorSetAllocateInfo allocInfoImgui{ *m_descriptorPoolImgui, 1, layoutsImgui };
        m_descriptorSetsImgui = static_cast<vk::Device>(m_device).allocateDescriptorSetsUnique(allocInfoImgui);
    }

    void ImguiBaseDemo::uploadFonts()
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
        auto & sampler{ reinterpret_cast<vk::UniqueSampler &>(m_fontSampler) };
        vk::DescriptorImageInfo descImage[1] = { { *sampler, *m_imguiFontImageView, vk::ImageLayout::eShaderReadOnlyOptimal } };
        vk::WriteDescriptorSet writeDesc(*m_descriptorSetsImgui[0], 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, descImage);
        static_cast<vk::Device>(m_device).updateDescriptorSets(writeDesc, nullptr);

        // Create the Upload Buffer:
        vk::BufferCreateInfo bufferInfo{ {}, upload_size, vk::BufferUsageFlagBits::eTransferSrc };
        auto uploadBuffer{ static_cast<vk::Device>(m_device).createBufferUnique(bufferInfo) };

        const auto bufferReq{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*uploadBuffer) };
        m_bufferMemoryAlignmentImgui = m_bufferMemoryAlignmentImgui > bufferReq.alignment ? m_bufferMemoryAlignmentImgui : bufferReq.alignment;
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

        uploadBuffer.reset(nullptr);
    }

    void ImguiBaseDemo::imguiRenderDrawLists(ImDrawData * draw_data)
    {
        if (draw_data->TotalVtxCount == 0)
        {
            return;
        }

        auto & io = ImGui::GetIO();

        // Create the Vertex Buffer:
        auto vertexSize{ draw_data->TotalVtxCount * sizeof(ImDrawVert) };
        if (!m_vertexBufferImgui || m_vertexBufferSize < vertexSize)
        {
            if (m_vertexBufferImgui)
            {
                m_vertexBufferImgui.reset(nullptr);
            }

            if (m_vertexBufferMemoryImgui)
            {
                m_vertexBufferMemoryImgui.reset(nullptr);
            }

            const auto vertexBufferSize{ ((vertexSize - 1) / m_bufferMemoryAlignmentImgui + 1) * m_bufferMemoryAlignmentImgui };
            vk::BufferCreateInfo bufferInfo{ {}, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer };
            m_vertexBufferImgui = static_cast<vk::Device>(m_device).createBufferUnique(bufferInfo);

            const auto req{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*m_vertexBufferImgui) };
            m_bufferMemoryAlignmentImgui = m_bufferMemoryAlignmentImgui > req.alignment ? m_bufferMemoryAlignmentImgui : req.alignment;

            vk::MemoryAllocateInfo allocInfo{ req.size, getImguiMemoryType(static_cast<vk::PhysicalDevice>(m_instance.getPhysicalDevice()), vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits) };
            m_vertexBufferMemoryImgui = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);

            static_cast<vk::Device>(m_device).bindBufferMemory(*m_vertexBufferImgui, *m_vertexBufferMemoryImgui, 0);

            m_vertexBufferSize = vertexBufferSize;
        }

        // Create the Index Buffer:
        auto indexSize{ draw_data->TotalIdxCount * sizeof(ImDrawIdx) };
        if (!m_indexBufferImgui || m_indexBufferSize < indexSize)
        {
            if (m_indexBufferImgui)
            {
                m_indexBufferImgui.reset(nullptr);
            }

            if (m_indexBufferMemoryImgui)
            {
                m_indexBufferMemoryImgui.reset(nullptr);
            }

            const auto indexBufferSize{ ((indexSize - 1) / m_bufferMemoryAlignmentImgui + 1) * m_bufferMemoryAlignmentImgui };
            vk::BufferCreateInfo bufferInfo{ {}, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer };
            m_indexBufferImgui = static_cast<vk::Device>(m_device).createBufferUnique(bufferInfo);

            const auto req{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*m_indexBufferImgui) };
            m_bufferMemoryAlignmentImgui = m_bufferMemoryAlignmentImgui > req.alignment ? m_bufferMemoryAlignmentImgui : req.alignment;

            vk::MemoryAllocateInfo allocInfo{ req.size, getImguiMemoryType(static_cast<vk::PhysicalDevice>(m_instance.getPhysicalDevice()), vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits) };
            m_indexBufferMemoryImgui = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);

            static_cast<vk::Device>(m_device).bindBufferMemory(*m_indexBufferImgui, *m_indexBufferMemoryImgui, 0);

            m_indexBufferSize = indexBufferSize;
        }

        // Upload Vertex and index Data:
        auto vtx_dst = reinterpret_cast<ImDrawVert *>(m_device.mapMemory(m_vertexBufferMemoryImgui, vertexSize));
        auto idx_dst = reinterpret_cast<ImDrawIdx *>(m_device.mapMemory(m_indexBufferMemoryImgui, indexSize));
        
        for (auto n = 0; n < draw_data->CmdListsCount; ++n)
        {
            const ImDrawList * cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }

        std::vector<vk::MappedMemoryRange> ranges =
        {
            { *m_vertexBufferMemoryImgui, 0, VK_WHOLE_SIZE },
            { *m_indexBufferMemoryImgui, 0, VK_WHOLE_SIZE },
        };
        static_cast<vk::Device>(m_device).flushMappedMemoryRanges(ranges);

        m_device.unmapMemory(m_vertexBufferMemoryImgui);
        m_device.unmapMemory(m_indexBufferMemoryImgui);

        // Bind pipeline and descriptor sets:
        m_commandBufferImguiPtr->bindPipeline(m_graphicsPipelineImgui);
        m_commandBufferImguiPtr->bindDescriptorSet(m_pipelineLayoutImgui, m_descriptorSetsImgui[0]);

        // Bind Vertex And Index Buffer:
        m_commandBufferImguiPtr->bindVertexBuffer(m_vertexBufferImgui);
        m_commandBufferImguiPtr->bindIndexBuffer(m_indexBufferImgui);

        // Setup viewport:
        m_commandBufferImguiPtr->setViewport({ 0.f, 0.f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.f, 1.f });

        // Setup scale and translation:
        m_commandBufferImguiPtr->pushConstants<float>(m_pipelineLayoutImgui, vk::ShaderStageFlagBits::eVertex, 0, std::vector<float>{ 2.f / io.DisplaySize.x, 2.f / io.DisplaySize.y });
        m_commandBufferImguiPtr->pushConstants<float>(m_pipelineLayoutImgui, vk::ShaderStageFlagBits::eVertex, 2, std::vector<float>{ -1.f, -1.f });

        // Render the command lists:
        auto vtxOffset = 0;
        auto idxOffset = 0;
        for (auto n = 0; n < draw_data->CmdListsCount; ++n)
        {
            const ImDrawList * cmdList = draw_data->CmdLists[n];
            for (auto cmdI = 0; cmdI < cmdList->CmdBuffer.Size; ++cmdI)
            {
                auto pcmd = &cmdList->CmdBuffer[cmdI];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmdList, pcmd);
                }
                else
                {
                    vk::Offset2D offset{ static_cast<int32_t>(pcmd->ClipRect.x) > 0 ? static_cast<int32_t>(pcmd->ClipRect.x) : 0, static_cast<int32_t>(pcmd->ClipRect.y) > 0 ? static_cast<int32_t>(pcmd->ClipRect.y) : 0 };
                    vk::Extent2D extent{ static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x), static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y + 1) }; // FIXME: Why +1 here?
                    vk::Rect2D scissor{ offset, extent };
                    m_commandBufferImguiPtr->setScissor(scissor);
                    m_commandBufferImguiPtr->drawIndexed(pcmd->ElemCount, 1, idxOffset, vtxOffset);
                }

                idxOffset += pcmd->ElemCount;
            }

            vtxOffset += cmdList->VtxBuffer.Size;
        }
    }

    void ImguiBaseDemo::drawFrame(uint32_t imageIndex, const vk::UniqueSemaphore & renderFinishedSemaphore, const vk::UniqueSemaphore & renderImguiFinishedSemaphore)
    {
        m_commandBufferImguiPtr.reset();
        m_commandBufferImguiPtr = std::make_unique<CommandBuffer>(m_device.allocateCommandBuffer(m_commandPool));
        m_commandBufferImguiPtr->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        std::vector<vk::ClearValue> clearValues{ vk::ClearColorValue{ std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f } } };
        m_commandBufferImguiPtr->beginRenderPass(m_renderPassImgui, m_swapChainFramebuffers[imageIndex], { { 0, 0 }, m_swapchain.getExtent() }, clearValues);

        imguiRenderDrawLists(ImGui::GetDrawData());

        m_commandBufferImguiPtr->endRenderPass();
        m_commandBufferImguiPtr->end();

        m_queue.submit(*m_commandBufferImguiPtr.get(), renderFinishedSemaphore, renderImguiFinishedSemaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
    }

    void ImguiBaseDemo::imguiNewFrame()
    {
        auto & io{ ImGui::GetIO() };

        // Setup display size (every frame to accommodate for window resizing)
        int w, h;
        int display_w, display_h;
        std::tie(w, h) = m_window.getSize();
        std::tie(display_w, display_h) = m_window.getFramebufferSize();
        io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
        io.DisplayFramebufferScale = ImVec2(w > 0 ? static_cast<float>(display_w) / w : 0, h > 0 ? static_cast<float>(display_h) / h : 0);

        // Setup time step
        const auto current_time = glfwGetTime();
        io.DeltaTime = m_imguiTime > 0.0 ? static_cast<float>(current_time - m_imguiTime) : static_cast<float>(1.0f / 60.0f);
        m_imguiTime = current_time;

        // Setup inputs
        // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
        if (m_window.isFocused())
        {
            double mouse_x, mouse_y;
            std::tie(mouse_x, mouse_y) = m_window.getCursorPos();
            io.MousePos = ImVec2(static_cast<float>(mouse_x), static_cast<float>(mouse_y));   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
        }
        else
        {
            io.MousePos = ImVec2(-1, -1);
        }

        for (auto i = 0; i < 3; i++)
        {
            io.MouseDown[i] = m_imguiMousePressed[i] || m_window.getMouseButtonState(i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            m_imguiMousePressed[i] = false;
        }

        io.MouseWheel = m_imguiMouseWheel;
        m_imguiMouseWheel = 0.0f;

        // Hide OS mouse cursor if ImGui is drawing it
        m_window.setInputMode(GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

        // Start the frame
        ImGui::NewFrame();
    }

    void ImguiBaseDemo::imguiMouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
    {
        if (action == GLFW_PRESS && button >= 0 && button < 3)
        {
            m_imguiMousePressed[button] = true;
        }
    }

    void ImguiBaseDemo::imguiScrollCallback(GLFWwindow * window, double xoffset, double yoffset)
    {
        m_imguiMouseWheel += static_cast<float>(yoffset); // Use fractional mouse wheel, 1.0 unit 5 lines.
    }

    void ImguiBaseDemo::imguiKeyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) const
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
