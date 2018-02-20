#include "coordinatesDemo.hpp"

#include <tinyobjloader/tiny_obj_loader.h>
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <vw/modelLoader.hpp>

#include "shader.hpp"

namespace bmvk
{
    const std::string K_VERTEX_SHADER_PATH{ "../shaders/coordinates.vert.spv" };
    const std::string K_COLOR_FRAGMENT_SHADER_PATH{ "../shaders/coordinates_color.frag.spv" };
    const std::string K_NORMAL_FRAGMENT_SHADER_PATH{ "../shaders/coordinates_normal.frag.spv" };
    const std::string K_WORLDNORMAL_FRAGMENT_SHADER_PATH{ "../shaders/coordinates_worldNormal.frag.spv" };
    const std::string K_VIEWPOS_FRAGMENT_SHADER_PATH{ "../shaders/coordinates_viewPos.frag.spv" };

    const std::string K_MODEL_PATH{ "../models/stanford_dragon/dragon.obj" };

    CoordinatesDemo::CoordinatesDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
        : ImguiBaseDemo{ enableValidationLayers, width, height, "Coordinates Demo", DebugReport::ReportLevel::WarningsAndAbove },
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
        loadCube();
        loadDragon();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
    }

    void CoordinatesDemo::run()
    {
        while (!m_window.shouldClose())
        {
            /*
            * CPU
            */

            m_window.pollEvents();

            updateUniformBuffer();
            imguiNewFrame();

            // 1. Show a simple window
            // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
            {
                ImGui::SetNextWindowPos(ImVec2(20, 20));
                ImGui::SetNextWindowSize(ImVec2(400, 50), ImGuiCond_Always);
                ImGui::Begin("Performance");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", m_avgFrameTime / 1000.0, m_avgFps);
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

    void CoordinatesDemo::recreateSwapChain()
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
        m_colorPipeline.reset(nullptr);
        m_normalPipeline.reset(nullptr);
        m_pipelineLayout.reset(nullptr);
        m_renderPass.reset(nullptr);

        ImguiBaseDemo::recreateSwapChain();

        createRenderPass();
        createPipelines();
        createDepthResources();
        createFramebuffers();
        createCommandBuffers();
    }

    void CoordinatesDemo::setupCamera()
    {
        const glm::vec3 pos{ 0.f, 0.f, 5.f };
        const glm::vec3 dir{ 0.f, 0.f, -1.f };
        const glm::vec3 up{ 0.f, 1.f, 0.f };
        const auto extent{ m_swapchain.getExtent() };
        m_camera = vw::util::Camera(pos, dir, up, 45.f, extent.width / static_cast<float>(extent.height), 0.01f, std::numeric_limits<float>::infinity());
    }

    void CoordinatesDemo::createDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
        m_descriptorSetLayout = m_device.createDescriptorSetLayout({ uboLayoutBinding });
    }

    void CoordinatesDemo::createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentDescription depthAttachment{ {}, m_instance.getPhysicalDevice().findDepthFormat(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        vk::AttachmentReference depthAttachmentRef{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef, nullptr, &depthAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,{}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        std::vector<vk::AttachmentDescription> vec{ colorAttachment, depthAttachment };
        RenderPassCreateInfo renderPassInfo{ {}, vec, subpass, dependency };
        m_renderPass = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);
    }

    void CoordinatesDemo::createPipelines()
    {
        const Shader vertShader{ K_VERTEX_SHADER_PATH, m_device };
        const Shader colorFragShader{ K_COLOR_FRAGMENT_SHADER_PATH, m_device };
        const Shader normalFragShader{ K_NORMAL_FRAGMENT_SHADER_PATH, m_device };
        const Shader worldNormalFragShader{ K_WORLDNORMAL_FRAGMENT_SHADER_PATH, m_device };
        const Shader viewPosFragShader{ K_VIEWPOS_FRAGMENT_SHADER_PATH, m_device };
        const auto vertShaderStageInfo{ vertShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex) };
        const auto colorFragShaderStageInfo{ colorFragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        const auto normalFragShaderStageInfo{ normalFragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        const auto worldNormalFragShaderStageInfo{ worldNormalFragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        const auto viewPosFragShaderStageInfo{ viewPosFragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        vk::PipelineShaderStageCreateInfo colorShaderStages[] = { vertShaderStageInfo, colorFragShaderStageInfo };
        vk::PipelineShaderStageCreateInfo normalShaderStages[] = { vertShaderStageInfo, normalFragShaderStageInfo };
        vk::PipelineShaderStageCreateInfo worldNormalShaderStages[] = { vertShaderStageInfo, worldNormalFragShaderStageInfo };
        vk::PipelineShaderStageCreateInfo viewPosShaderStages[] = { vertShaderStageInfo, viewPosFragShaderStageInfo };

        auto bindingDescription = vw::scene::Vertex::getBindingDescription();
        auto attributeDescriptions = vw::scene::Vertex::getAttributeDescriptions();
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
        m_pipelineLayout = m_device.createPipelineLayout({ *m_descriptorSetLayout });

        vk::GraphicsPipelineCreateInfo colorPipelineInfo(vk::PipelineCreateFlagBits::eAllowDerivatives, 2, colorShaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, &dynamicState, *m_pipelineLayout, *m_renderPass, 0, nullptr, -1);
        m_colorPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, colorPipelineInfo);

        vk::GraphicsPipelineCreateInfo normalPipelineInfo(vk::PipelineCreateFlagBits::eDerivative, 2, normalShaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, &dynamicState, *m_pipelineLayout, *m_renderPass, 0, *m_colorPipeline, -1);
        m_normalPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, normalPipelineInfo);

        vk::GraphicsPipelineCreateInfo worldNormalPipelineInfo(vk::PipelineCreateFlagBits::eDerivative, 2, worldNormalShaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, &dynamicState, *m_pipelineLayout, *m_renderPass, 0, *m_colorPipeline, -1);
        m_worldNormalPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, worldNormalPipelineInfo);

        vk::GraphicsPipelineCreateInfo viewPosPipelineInfo(vk::PipelineCreateFlagBits::eDerivative, 2, viewPosShaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, &dynamicState, *m_pipelineLayout, *m_renderPass, 0, *m_colorPipeline, -1);
        m_viewPosPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, viewPosPipelineInfo);
    }

    void CoordinatesDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.clear();
        for (auto & uniqueImageView : m_swapchain.getImageViews())
        {
            std::vector<vk::ImageView> imageViews{ *uniqueImageView, *m_depthImageView };
            m_swapChainFramebuffers.emplace_back(m_device.createFramebuffer(m_renderPass, imageViews, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1));
        }
    }

    void CoordinatesDemo::loadCube()
    {
        const auto posToCol = [](const glm::vec3 & pos) { return glm::vec3(std::max(0.f, pos.x), std::max(0.f, pos.y), std::max(0.f, pos.z)); };

        const auto createSide = [&posToCol](vw::scene::Model & model, const glm::vec3 & n, const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & d, const int i)
        {
            model.getVertices().push_back(vw::scene::Vertex{ a, n, posToCol(a), {} });
            model.getVertices().push_back(vw::scene::Vertex{ b, n, posToCol(b), {} });
            model.getVertices().push_back(vw::scene::Vertex{ c, n, posToCol(c), {} });
            model.getVertices().push_back(vw::scene::Vertex{ d, n, posToCol(d), {} });

            model.getIndices().push_back(i);
            model.getIndices().push_back(i + 1);
            model.getIndices().push_back(i + 2);
            model.getIndices().push_back(i);
            model.getIndices().push_back(i + 2);
            model.getIndices().push_back(i + 3);
        };

        // front
        createSide(m_cube, { 0.f, 0.f, 1.f }, { -1.f, -1.f, 1.f }, { 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, { -1.f, 1.f, 1.f }, 0);

        // right
        createSide(m_cube, { 1.f, 0.f, 0.f }, { 1.f, -1.f, 1.f }, { 1.f, -1.f, -1.f }, { 1.f, 1.f, -1.f }, { 1.f, 1.f, 1.f }, 4);

        // back
        createSide(m_cube, { 0.f, 0.f, -1.f }, { 1.f, -1.f, -1.f }, { -1.f, -1.f, -1.f }, { -1.f, 1.f, -1.f }, { 1.f, 1.f, -1.f }, 8);

        // left
        createSide(m_cube, { -1.f, 0.f, 0.f }, { -1.f, -1.f, -1.f }, { -1.f, -1.f, 1.f }, { -1.f, 1.f, 1.f }, { -1.f, 1.f, -1.f }, 12);

        // up
        createSide(m_cube, { 0.f, 1.f, 0.f }, { -1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f }, { 1.f, 1.f, -1.f }, { -1.f, 1.f, -1.f }, 16);

        // down
        createSide(m_cube, { 0.f, -1.f, 0.f }, { -1.f, -1.f, -1.f }, { 1.f, -1.f, -1.f }, { 1.f, -1.f, 1.f }, { -1.f, -1.f, 1.f }, 20);

        m_cube.createBuffers(static_cast<vk::Device>(m_device), static_cast<vk::PhysicalDevice>(m_instance.getPhysicalDevice()), m_commandPool, static_cast<vk::Queue>(m_queue));
    }

    void CoordinatesDemo::loadDragon()
    {
        vw::scene::ModelLoader ml;
        m_dragon = ml.loadModel(K_MODEL_PATH, vw::scene::ModelLoader::NormalCreation::Explicit);

        m_dragon.createBuffers(static_cast<vk::Device>(m_device), static_cast<vk::PhysicalDevice>(m_instance.getPhysicalDevice()), m_commandPool, static_cast<vk::Queue>(m_queue));
    }

    void CoordinatesDemo::createDepthResources()
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

    void CoordinatesDemo::createUniformBuffer()
    {
        const auto bufferSize{ sizeof(UniformBufferObject) };
        const auto uniformBufferUsageFlags{ vk::BufferUsageFlagBits::eUniformBuffer };
        const auto uniformBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        createBuffer(bufferSize, uniformBufferUsageFlags, uniformBufferMemoryPropertyFlags, m_uniformBuffer, m_uniformBufferMemory);
    }

    void CoordinatesDemo::createDescriptorPool()
    {
        vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
        std::vector<vk::DescriptorPoolSize> vec{ poolSize };
        m_descriptorPool = m_device.createDescriptorPool(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, vec);
    }

    void CoordinatesDemo::createDescriptorSet()
    {
        vk::DescriptorSetLayout layouts[] = { *m_descriptorSetLayout };
        vk::DescriptorSetAllocateInfo allocInfo{ *m_descriptorPool, 1, layouts };
        m_descriptorSets = static_cast<vk::Device>(m_device).allocateDescriptorSetsUnique(allocInfo);

        vk::DescriptorBufferInfo bufferInfo{ *m_uniformBuffer, 0, sizeof(UniformBufferObject) };
        WriteDescriptorSet descriptorWrite{ m_descriptorSets[0], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo };
        std::vector<vk::WriteDescriptorSet> vec{ descriptorWrite };
        m_device.updateDescriptorSets(vec);
    }

    void CoordinatesDemo::createCommandBuffers()
    {
        m_commandBuffers = m_device.allocateCommandBuffers(m_commandPool, static_cast<uint32_t>(m_swapChainFramebuffers.size()));
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            const auto & cmdBuffer{ m_commandBuffers[i] };
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            std::vector<vk::ClearValue> clearValues{ vk::ClearColorValue{ std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f } }, vk::ClearDepthStencilValue{ 1.f, 0 } };
            cmdBuffer.beginRenderPass(m_renderPass, m_swapChainFramebuffers[i], { { 0, 0 }, m_swapchain.getExtent() }, clearValues);
            const auto extent{ m_swapchain.getExtent() };
            vk::Viewport vp{ 0.f, 0.f, extent.width / 2.f, static_cast<float>(extent.height) / 2.f, 0.f, 1.f };
            cmdBuffer.setViewport(vp);
            cmdBuffer.bindPipeline(m_colorPipeline);
            cmdBuffer.bindDescriptorSet(m_pipelineLayout, m_descriptorSets[0]);
            const auto & cb_vk{ reinterpret_cast<const vk::UniqueCommandBuffer &>(cmdBuffer) };
            //m_cube.draw(cb_vk);
            m_dragon.draw(cb_vk);
            vp.setX(extent.width / 2.f);
            cmdBuffer.setViewport(vp);
            cmdBuffer.bindPipeline(m_normalPipeline);
            //m_cube.draw(cb_vk);
            m_dragon.draw(cb_vk);
            vp.setY(extent.height / 2.f);
            cmdBuffer.setViewport(vp);
            cmdBuffer.bindPipeline(m_worldNormalPipeline);
            //m_cube.draw(cb_vk);
            m_dragon.draw(cb_vk);
            vp.setX(0.f);
            cmdBuffer.setViewport(vp);
            cmdBuffer.bindPipeline(m_viewPosPipeline);
            //m_cube.draw(cb_vk);
            m_dragon.draw(cb_vk);
            cmdBuffer.endRenderPass();
            cmdBuffer.end();
        }
    }

    void CoordinatesDemo::updateUniformBuffer()
    {
        UniformBufferObject ubo;
        ubo.model = m_cube.getModelMatrix();
        const auto extent{ m_swapchain.getExtent() };
        m_camera.setRatio(extent.width / static_cast<float>(extent.height));
        ubo.view = m_camera.getViewMatrix();
        ubo.proj = m_camera.getProjMatrix();
        ubo.proj[1][1] *= -1;
        ubo.normal = glm::inverseTranspose(ubo.view * ubo.model);

        m_device.copyToMemory(m_uniformBufferMemory, ubo);
    }

    void CoordinatesDemo::drawFrame()
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
        auto swapchain{ static_cast<vk::SwapchainKHR>(m_swapchain) };
        auto success{ m_queue.present(waitSemaphore, swapchain, imageIndex) };
        if (!success)
        {
            recreateSwapChain();
        }
    }
}
