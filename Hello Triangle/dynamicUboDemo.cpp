#include "dynamicUboDemo.hpp"

#include <tinyobjloader/tiny_obj_loader.h>
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <vw/modelLoader.hpp>

#include "shader.hpp"

#include <iostream>
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

namespace bmvk
{
    const std::string K_VERTEX_SHADER_PATH{ "../shaders/dynamicUboDemo/dynamicUbo.vert.spv" };
    const std::string K_FRAGMENT_SHADER_PATH{ "../shaders/dynamicUboDemo/dynamicUbo.frag.spv" };

    void * alignedAlloc(size_t size, size_t alignment)
    {
        void * data = nullptr;
        data = _aligned_malloc(size, alignment);
        return data;
    }

    void alignedFree(void* data)
    {
        _aligned_free(data);
    }

    DynamicUboDemo::DynamicUboDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
        : ImguiBaseDemo{ enableValidationLayers, width, height, "DynamicUbo Demo", DebugReport::ReportLevel::WarningsAndAbove },
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
        createUniformBuffer();
        createDynamicUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
    }

    DynamicUboDemo::~DynamicUboDemo()
    {
        alignedFree(m_dynamicUniformBufferObject.model);
    }

    void DynamicUboDemo::run()
    {
        while (!m_window.shouldClose())
        {
            /*
            * CPU
            */

            m_window.pollEvents();

            updateUniformBuffer();
            updateDynamicUniformBuffer();
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

    void DynamicUboDemo::recreateSwapChain()
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

    void DynamicUboDemo::setupCamera()
    {
        const glm::vec3 pos{ 0.f, 0.f, 25.f };
        const glm::vec3 dir{ 0.f, 0.f, -1.f };
        const glm::vec3 up{ 0.f, 1.f, 0.f };
        const auto extent{ m_swapchain.getExtent() };
        m_camera = vw::util::Camera(pos, dir, up, 45.f, extent.width / static_cast<float>(extent.height), 0.01f, std::numeric_limits<float>::infinity());
    }

    void DynamicUboDemo::createDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
        vk::DescriptorSetLayoutBinding dynamicUboLayoutBinding{ 1, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex };
        m_descriptorSetLayout = m_device.createDescriptorSetLayout({ uboLayoutBinding, dynamicUboLayoutBinding });
    }

    void DynamicUboDemo::createRenderPass()
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

    void DynamicUboDemo::createPipelines()
    {
        const Shader vertShader{ K_VERTEX_SHADER_PATH, m_device };
        const Shader fragShader{ K_FRAGMENT_SHADER_PATH, m_device };
        const auto vertShaderStageInfo{ vertShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex) };
        const auto fragShaderStageInfo{ fragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = vw::util::Vertex::getBindingDescription();
        auto attributeDescriptions = vw::util::Vertex::getAttributeDescriptions();
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

        vk::GraphicsPipelineCreateInfo colorPipelineInfo({}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, &dynamicState, *m_pipelineLayout, *m_renderPass, 0, nullptr, -1);
        m_pipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, colorPipelineInfo);
    }

    void DynamicUboDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.clear();
        for (auto & uniqueImageView : m_swapchain.getImageViews())
        {
            std::vector<vk::ImageView> imageViews{ *uniqueImageView, *m_depthImageView };
            m_swapChainFramebuffers.emplace_back(m_device.createFramebuffer(m_renderPass, imageViews, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1));
        }
    }

    void DynamicUboDemo::loadCube()
    {
        const auto posToCol = [](const glm::vec3 & pos) { return glm::vec3(std::max(0.f, pos.x), std::max(0.f, pos.y), std::max(0.f, pos.z)); };

        const auto createSide = [&posToCol](vw::util::Model & model, const glm::vec3 & n, const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & d, const int i)
        {
            model.getVertices().push_back(vw::util::Vertex{ a, n, posToCol(a),{} });
            model.getVertices().push_back(vw::util::Vertex{ b, n, posToCol(b),{} });
            model.getVertices().push_back(vw::util::Vertex{ c, n, posToCol(c),{} });
            model.getVertices().push_back(vw::util::Vertex{ d, n, posToCol(d),{} });

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

    void DynamicUboDemo::createDepthResources()
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

    void DynamicUboDemo::createUniformBuffer()
    {
        const auto bufferSize{ sizeof(UniformBufferObject) };
        const auto uniformBufferUsageFlags{ vk::BufferUsageFlagBits::eUniformBuffer };
        const auto uniformBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        createBuffer(bufferSize, uniformBufferUsageFlags, uniformBufferMemoryPropertyFlags, m_uniformBuffer, m_uniformBufferMemory);
    }

    void DynamicUboDemo::createDynamicUniformBuffer()
    {
        size_t minUboAlignment = static_cast<vk::PhysicalDevice>(m_instance.getPhysicalDevice()).getProperties().limits.minUniformBufferOffsetAlignment;
        m_dynamicAlignment = sizeof(glm::mat4);
        if (minUboAlignment > 0)
        {
            m_dynamicAlignment = (m_dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        const auto bufferSize{ k_objectInstances * m_dynamicAlignment };

        m_dynamicUniformBufferObject.model = static_cast<glm::mat4 *>(alignedAlloc(bufferSize, m_dynamicAlignment));
        if (m_dynamicUniformBufferObject.model == nullptr)
        {
            throw std::runtime_error("model must not be null");
        }

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible, m_dynamicUniformBuffer, m_dynamicUniformBufferMemory);

        std::default_random_engine rndEngine(static_cast<unsigned int>(time(nullptr)));
        std::normal_distribution<float> rndDist(-1.0f, 1.0f);
        for (uint32_t i = 0; i < k_objectInstances; ++i) {
            m_rotations[i] = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine)) * 2.f * static_cast<float>(M_PI);
            m_rotationSpeeds[i] = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
        }

        updateDynamicUniformBuffer();
    }

    void DynamicUboDemo::createDescriptorPool()
    {
        vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
        vk::DescriptorPoolSize poolSize2{ vk::DescriptorType::eUniformBufferDynamic, 1 };
        std::vector<vk::DescriptorPoolSize> vec{ poolSize, poolSize2 };
        m_descriptorPool = m_device.createDescriptorPool(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, vec);
    }

    void DynamicUboDemo::createDescriptorSet()
    {
        vk::DescriptorSetLayout layouts[] = { *m_descriptorSetLayout };
        vk::DescriptorSetAllocateInfo allocInfo{ *m_descriptorPool, 1, layouts };
        m_descriptorSets = static_cast<vk::Device>(m_device).allocateDescriptorSetsUnique(allocInfo);

        vk::DescriptorBufferInfo bufferInfo{ *m_uniformBuffer, 0, sizeof(UniformBufferObject) };
        WriteDescriptorSet descriptorWrite{ m_descriptorSets[0], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo };
        vk::DescriptorBufferInfo dynamicBufferInfo{ *m_dynamicUniformBuffer, 0, sizeof(DynamicUniformBufferObject) };
        WriteDescriptorSet descriptorWrite2{ m_descriptorSets[0], 1, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &dynamicBufferInfo };
        std::vector<vk::WriteDescriptorSet> vec{ descriptorWrite, descriptorWrite2 };
        m_device.updateDescriptorSets(vec);
    }

    void DynamicUboDemo::createCommandBuffers()
    {
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
            cmdBuffer.bindPipeline(m_pipeline);
            const auto & cb_vk{ reinterpret_cast<const vk::UniqueCommandBuffer &>(cmdBuffer) };
            m_cube.drawInstanced(cb_vk, m_pipelineLayout, m_descriptorSets[0], k_objectInstances, m_dynamicAlignment);
            cmdBuffer.endRenderPass();
            cmdBuffer.end();
        }
    }

    void DynamicUboDemo::updateUniformBuffer()
    {
        UniformBufferObject ubo;
        const auto extent{ m_swapchain.getExtent() };
        m_camera.setRatio(extent.width / static_cast<float>(extent.height));
        ubo.view = m_camera.getViewMatrix();
        ubo.proj = m_camera.getProjMatrix();
        ubo.proj[1][1] *= -1;

        m_device.copyToMemory(m_uniformBufferMemory, ubo);
    }

    void DynamicUboDemo::updateDynamicUniformBuffer()
    {
        // Update at max. 60 fps
        m_animationTimer += static_cast<float>(m_currentFrameTime);
        if (m_animationTimer <= 1.0f / 60.0f) {
            return;
        }

        // Dynamic ubo with per-object model matrices indexed by offsets in the command buffer
        uint32_t dim = static_cast<uint32_t>(pow(k_objectInstances, 1.0f / 3.0f));
        glm::vec3 offset(5.0f);

        for (uint32_t x = 0; x < dim; ++x)
        {
            for (uint32_t y = 0; y < dim; ++y)
            {
                for (uint32_t z = 0; z < dim; ++z)
                {
                    uint32_t index = x * dim * dim + y * dim + z;

                    // Aligned offset
                    glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + (index * m_dynamicAlignment)));

                    // Update rotations
                    m_rotations[index] += m_animationTimer * m_rotationSpeeds[index];

                    // Update matrices
                    glm::vec3 pos = glm::vec3(-((dim * offset.x) / 2.0f) + offset.x / 2.0f + x * offset.x, -((dim * offset.y) / 2.0f) + offset.y / 2.0f + y * offset.y, -((dim * offset.z) / 2.0f) + offset.z / 2.0f + z * offset.z);
                    *modelMat = glm::translate(glm::mat4(1.0f), pos);
                    *modelMat = glm::rotate(*modelMat, m_rotations[index].x, glm::vec3(1.0f, 1.0f, 0.0f));
                    *modelMat = glm::rotate(*modelMat, m_rotations[index].y, glm::vec3(0.0f, 1.0f, 0.0f));
                    *modelMat = glm::rotate(*modelMat, m_rotations[index].z, glm::vec3(0.0f, 0.0f, 1.0f));
                    //*modelMat = glm::mat4(1.f);
                }
            }
        }

        m_animationTimer = 0.0f;

        const auto bufSize{ k_objectInstances * m_dynamicAlignment };
        auto data{ m_device.mapMemory(m_dynamicUniformBufferMemory, bufSize) };
        memcpy(data, m_dynamicUniformBufferObject.model, bufSize);
        vk::MappedMemoryRange mmr{ *m_dynamicUniformBufferMemory, 0, bufSize };
        static_cast<vk::Device>(m_device).flushMappedMemoryRanges(mmr);
        m_device.unmapMemory(m_dynamicUniformBufferMemory);
    }

    void DynamicUboDemo::drawFrame()
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
