#include "pushConstantDemo.hpp"

#include <tinyobjloader/tiny_obj_loader.h>
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <vw/modelLoader.hpp>

#include "shader.hpp"

namespace bmvk
{
    const std::string K_VERTEX_SHADER_PATH{ "../shaders/pushConstantDemo/vertex.vert.spv" };
    const std::string K_FRAGMENT_SHADER_PATH{ "../shaders/pushConstantDemo/fragment.frag.spv" };
    const std::string K_SCENE_PATH{ "../models/samplescene/samplescene.dae" };

    constexpr auto r{ 7.5f };
    auto y{ 0.f };

    PushConstantDemo::PushConstantDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
        : ImguiBaseDemo{ enableValidationLayers, width, height, "PushConstant Demo", DebugReport::ReportLevel::Everything },
        m_imageAvailableSemaphore{ m_device.createSemaphore() },
        m_renderFinishedSemaphore{ m_device.createSemaphore() },
        m_renderImguiFinishedSemaphore{ m_device.createSemaphore() }
    {
        setupCamera();

        createDescriptorSetLayout();
        createRenderPass();
        createPipelines();
        createDepthResources();
        createFramebuffers();
        loadScene();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
    }

    void PushConstantDemo::run()
    {
        while (!m_window.shouldClose())
        {
            /*
            * CPU
            */

            m_window.pollEvents();

            recreateCommandBuffers();

            updateUniformBuffer();
            imguiNewFrame();

            {
                ImGui::SetNextWindowPos(ImVec2(20, 20));
                ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Once);
                ImGui::Begin("Performance");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", m_avgFrameTime / 1000.0, m_avgFps);
                ImGui::SliderFloat("Light Y", &y, -4.f, 10.f, "%.1f");
                ImGui::SliderFloat("Shiniess", &m_shininess, 1.f, 2048.f, "%.0f", 2.f);
                ImGui::SliderFloat("Ambient White", &m_ambientWhite, 0.f, 0.2f);
                ImGui::SliderFloat("Screen Gamma", &m_screenGamma, 0.1f, 4.f, "%.1f");
                ImGui::SliderFloat("Maximal Light Distance", &m_maxLightDist, 1.f, 20.f, "%.1f");
                ImGui::End();
            }

            ImGui::Render();

            /*
            * GPU
            */

            drawFrame();
        }

        m_device.waitIdle();
    }

    void PushConstantDemo::recreateSwapChain()
    {
        m_device.waitIdle();

        for (auto & fb : m_swapChainFramebuffers)
        {
            fb.reset(nullptr);
        }

        m_commandBuffers.clear();
        m_depthImageView.reset(nullptr);
        m_depthImage.reset(nullptr);
        m_depthImageMemory.reset(nullptr);
        m_pipeline.reset(nullptr);
        m_pipelineLayout.reset(nullptr);
        m_renderPass.reset(nullptr);

        ImguiBaseDemo::recreateSwapChain();

        createRenderPass();
        createPipelines();
        createDepthResources();
        createFramebuffers();
        createCommandBuffers();
    }

    void PushConstantDemo::setupCamera()
    {
        const glm::vec3 pos{ 0.f, 0.f, 25.f };
        const glm::vec3 dir{ 0.f, 0.f, -1.f };
        const glm::vec3 up{ 0.f, 1.f, 0.f };
        m_camera = vw::util::Camera(pos, dir, up, 45.f, m_swapchain.getRatio(), 0.01f, std::numeric_limits<float>::infinity());
    }

    void PushConstantDemo::createDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
        m_descriptorSetLayout = m_device.createDescriptorSetLayout({ uboLayoutBinding });
    }

    void PushConstantDemo::createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentDescription depthAttachment{ {}, m_instance.getPhysicalDevice().findDepthFormat(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        vk::AttachmentReference depthAttachmentRef{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef, nullptr, &depthAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,{}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        std::vector<vk::AttachmentDescription> vec{ colorAttachment, depthAttachment };
        RenderPassCreateInfo renderPassInfo{ {}, vec, subpass, dependency };
        m_renderPass = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createRenderPassUnique(renderPassInfo);
    }

    void PushConstantDemo::createPipelines()
    {
        const Shader vertShader{ K_VERTEX_SHADER_PATH, m_device };
        const Shader fragShader{ K_FRAGMENT_SHADER_PATH, m_device };
        const auto vertShaderStageInfo{ vertShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex) };
        const auto fragShaderStageInfo{ fragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = vw::scene::Vertex<VD>::getBindingDescription();
        auto attributeDescriptions = vw::scene::Vertex<VD>::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ PipelineVertexInputStateCreateInfo{ bindingDescription, attributeDescriptions } };
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList };
        vk::Viewport viewport;
        vk::Rect2D scissor;
        const auto viewportState{ m_swapchain.getPipelineViewportStateCreateInfo(viewport, scissor) };
        vk::PipelineRasterizationStateCreateInfo rasterizer{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, false, 0.f, 0.f, 0.f, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisampling;
        vk::PipelineDepthStencilStateCreateInfo depthStencil{ {}, true, true, vk::CompareOp::eLess, false, false };
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{ false, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlending{ {}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };
        std::vector<vk::DynamicState> dynamicStateEnables{ { vk::DynamicState::eViewport } };
        vk::PipelineDynamicStateCreateInfo dynamicState{ {}, static_cast<unsigned int>(dynamicStateEnables.size()), dynamicStateEnables.data() };
        vk::PushConstantRange pushConstantRange{ vk::ShaderStageFlagBits::eVertex, sizeof(glm::vec4) * 0, sizeof(glm::vec4) * 7 };
        m_pipelineLayout = m_device.createPipelineLayout({ *m_descriptorSetLayout }, {pushConstantRange});

        vk::GraphicsPipelineCreateInfo colorPipelineInfo({}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, &dynamicState, *m_pipelineLayout, *m_renderPass, 0, nullptr, -1);
        m_pipeline = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createGraphicsPipelineUnique(nullptr, colorPipelineInfo);
    }

    void PushConstantDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.clear();
        for (auto & uniqueImageView : m_swapchain.getImageViews())
        {
            std::vector<vk::ImageView> imageViews{ *uniqueImageView, *m_depthImageView };
            m_swapChainFramebuffers.emplace_back(m_device.createFramebuffer(m_renderPass, imageViews, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1));
        }
    }

    void PushConstantDemo::loadScene()
    {
        vw::scene::ModelLoader<VD> ml;
        m_model = ml.loadModel(K_SCENE_PATH, vw::scene::ModelLoader<VD>::NormalCreation::AssimpSmoothNormals);
        m_model.translate({ 0.f, -4.f, 0.f });
        m_model.createBuffers(reinterpret_cast<const vk::UniqueDevice &>(m_device), reinterpret_cast<const vk::PhysicalDevice &>(m_instance.getPhysicalDevice()), m_commandPool, reinterpret_cast<const vk::Queue &>(m_queue));
    }

    void PushConstantDemo::createDepthResources()
    {
        const auto depthFormat{ m_instance.getPhysicalDevice().findDepthFormat() };
        createImage(m_swapchain.getExtent().width, m_swapchain.getExtent().height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, m_depthImage, m_depthImageMemory);
        m_depthImageView = createImageView(m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        transitionImageLayout(cmdBuffer, m_depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_queue.waitIdle();
    }

    void PushConstantDemo::createUniformBuffer()
    {
        const auto bufferSize{ sizeof(UniformBufferObject) };
        const auto uniformBufferUsageFlags{ vk::BufferUsageFlagBits::eUniformBuffer };
        const auto uniformBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        createBuffer(bufferSize, uniformBufferUsageFlags, uniformBufferMemoryPropertyFlags, m_uniformBuffer, m_uniformBufferMemory);
    }

    void PushConstantDemo::createDescriptorPool()
    {
        vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
        std::vector<vk::DescriptorPoolSize> vec{ poolSize };
        m_descriptorPool = m_device.createDescriptorPool(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, vec);
    }

    void PushConstantDemo::createDescriptorSet()
    {
        vk::DescriptorSetLayout layouts[] = { *m_descriptorSetLayout };
        vk::DescriptorSetAllocateInfo allocInfo{ *m_descriptorPool, 1, layouts };
        m_descriptorSets = reinterpret_cast<const vk::UniqueDevice &>(m_device)->allocateDescriptorSetsUnique(allocInfo);

        vk::DescriptorBufferInfo bufferInfo{ *m_uniformBuffer, 0, sizeof(UniformBufferObject) };
        WriteDescriptorSet descriptorWrite{ m_descriptorSets[0], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo };
        std::vector<vk::WriteDescriptorSet> vec{ descriptorWrite };
        m_device.updateDescriptorSets(vec);
    }

    void PushConstantDemo::createCommandBuffers()
    {
        static auto m_timer{ 0.f };
        m_timer += static_cast<float>(m_currentFrameTime) * 0.1f;
        const auto sin_t{ std::sin(glm::radians(m_timer * 360)) };
        const auto cos_t{ std::cos(glm::radians(m_timer * 360)) };

        m_commandBuffers = m_device.allocateCommandBuffers(m_commandPool, static_cast<uint32_t>(m_swapChainFramebuffers.size()));
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            const auto & cmdBuffer{ m_commandBuffers[i] };
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            std::vector<vk::ClearValue> clearValues{ vk::ClearColorValue{ std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f } }, vk::ClearDepthStencilValue{ 1.f, 0 } };
            cmdBuffer.beginRenderPass(m_renderPass, m_swapChainFramebuffers[i], { { 0, 0 }, m_swapchain.getExtent() }, clearValues);
            const auto extent{ m_swapchain.getExtent() };
            vk::Viewport vp{ 0.f, 0.f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.f, 1.f };
            cmdBuffer.setViewport(vp);

            m_lightPositions.clear();
            m_lightPositions.emplace_back(m_shininess, m_ambientWhite, m_screenGamma, m_maxLightDist);
            m_lightPositions.emplace_back(r * 1.1 * sin_t, y, r * 1.1 * cos_t, 1.0f);
            m_lightPositions.emplace_back(-r * sin_t, y, -r * cos_t, 1.0f);
            m_lightPositions.emplace_back(r * 0.85f * sin_t, y, -sin_t * 2.5f, 1.5f);
            m_lightPositions.emplace_back(0.0f, y, r * 1.25f * cos_t, 1.5f);
            m_lightPositions.emplace_back(r * 2.25f * cos_t, y, 0.0f, 1.25f);
            m_lightPositions.emplace_back(r * 2.5f * cos_t, y, r * 2.5f * sin_t, 1.25f);
            cmdBuffer.pushConstants(m_pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0u, m_lightPositions);

            cmdBuffer.bindPipeline(m_pipeline);
            cmdBuffer.bindDescriptorSet(m_pipelineLayout, m_descriptorSets[0]);
            const auto & cb_vk{ reinterpret_cast<const vk::UniqueCommandBuffer &>(cmdBuffer) };
            m_model.draw(cb_vk);
            cmdBuffer.endRenderPass();
            cmdBuffer.end();
        }
    }

    void PushConstantDemo::recreateCommandBuffers()
    {
        m_queue.waitIdle();
        m_commandBuffers.clear();
        createCommandBuffers();
    }

    void PushConstantDemo::updateUniformBuffer()
    {
        UniformBufferObject ubo;
        const auto extent{ m_swapchain.getExtent() };
        m_camera.setRatio(extent.width / static_cast<float>(extent.height));
        ubo.model = m_model.getModelMatrix();
        ubo.view = m_camera.getViewMatrix();
        ubo.proj = m_camera.getProjMatrix();
        ubo.proj[1][1] *= -1;
        ubo.normal = glm::inverseTranspose(ubo.view * ubo.model);

        m_device.copyToMemory(m_uniformBufferMemory, ubo);
    }

    void PushConstantDemo::drawFrame()
    {
        m_queue.waitIdle();
        timing(false);

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
        ImguiBaseDemo::drawFrame(imageIndex, m_renderFinishedSemaphore, m_renderImguiFinishedSemaphore);

        auto waitSemaphore{ *m_renderImguiFinishedSemaphore };
        auto swapchain{ *reinterpret_cast<const vk::UniqueSwapchainKHR &>(m_swapchain) };
        auto success{ m_queue.present(waitSemaphore, swapchain, imageIndex) };
        if (!success)
        {
            recreateSwapChain();
        }
    }
}
