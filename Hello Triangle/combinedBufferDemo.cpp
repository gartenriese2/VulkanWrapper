#include "combinedBufferDemo.hpp"

#define STB_IMAGE_IMPLEMENTATION // TODO
#include <stb/stb_image.h>
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.inl>

#include "shader.hpp"
#include "bufferFactory.hpp"

namespace bmvk
{
    CombinedBufferDemo::CombinedBufferDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
      : ImguiBaseDemo{ enableValidationLayers, width, height, "CombinedBuffer Demo", true },
        m_textureSampler{ m_device.createSampler(true) },
        m_imageAvailableSemaphore{ m_device.createSemaphore() },
        m_renderFinishedSemaphore{ m_device.createSemaphore() },
        m_renderImguiFinishedSemaphore{ m_device.createSemaphore() }
    {
        createDescriptorSetLayout();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createTextureImage();
        createTextureImageView();
        createCombinedBuffer();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
    }

    void CombinedBufferDemo::run()
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

    void CombinedBufferDemo::recreateSwapChain()
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

        ImguiBaseDemo::recreateSwapChain();

        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }

    void CombinedBufferDemo::createDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
        vk::DescriptorSetLayoutBinding samplerLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };
        std::vector<vk::DescriptorSetLayoutBinding> vec{ uboLayoutBinding, samplerLayoutBinding };
        DescriptorSetLayoutCreateInfo layoutInfo{ vec };
        m_descriptorSetLayout = static_cast<vk::Device>(m_device).createDescriptorSetLayoutUnique(layoutInfo);
    }

    void CombinedBufferDemo::createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,{}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        RenderPassCreateInfo renderPassInfo{ {}, colorAttachment, subpass, dependency };
        m_renderPass = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);
    }

    void CombinedBufferDemo::createGraphicsPipeline()
    {
        const Shader vertShader{ "../shaders/texture.vert.spv", m_device };
        const Shader fragShader{ "../shaders/texture.frag.spv", m_device };
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

        vk::GraphicsPipelineCreateInfo pipelineInfo({}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending, nullptr, *m_pipelineLayout, *m_renderPass, 0, nullptr, -1);
        m_graphicsPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, pipelineInfo);
    }

    void CombinedBufferDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.clear();
        for (auto & uniqueImageView : m_swapchain.getImageViews())
        {
            auto imageView{ *uniqueImageView };
            m_swapChainFramebuffers.emplace_back(m_device.createFramebuffer(m_renderPass, imageView, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1));
        }
    }

    void CombinedBufferDemo::createTextureImage()
    {
        int texWidth, texHeight, texChannels;
        auto * pixels = stbi_load("../textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        StagingBuffer stagingBuffer{ m_bufferFactory.createStagingBuffer(imageSize) };
        stagingBuffer.fill(pixels, imageSize);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_textureImage, m_textureImageMemory);

        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        transitionImageLayout(cmdBuffer, m_textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        stagingBuffer.buffer.copyToImage(cmdBuffer, m_textureImage, texWidth, texHeight);
        transitionImageLayout(cmdBuffer, m_textureImage, vk::Format::eB8G8R8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_queue.waitIdle();
    }

    void CombinedBufferDemo::createTextureImageView()
    {
        m_textureImageView = createImageView(m_textureImage, vk::Format::eR8G8B8A8Unorm);
    }

    void CombinedBufferDemo::createCombinedBuffer()
    {
        const auto vertexBufferSize{ sizeof vertices[0] * vertices.size() };
        const auto indexBufferSize{ sizeof indices[0] * indices.size() };

        auto vertexStagingBuffer{ m_bufferFactory.createStagingBuffer(vertexBufferSize) };
        vertexStagingBuffer.fill(vertices.data(), vertexBufferSize);
        auto indexStagingBuffer{ m_bufferFactory.createStagingBuffer(indexBufferSize) };
        indexStagingBuffer.fill(indices.data(), indexBufferSize);

        /*auto stagingBuffer{ m_bufferFactory.createStagingBuffer(vertexBufferSize + indexBufferSize) };
        stagingBuffer.fill(vertices.data(), vertexBufferSize);
        stagingBuffer.fill(indices.data(), indexBufferSize, vertexBufferSize);*/

        const auto vertexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer };
        const auto indexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer };
        const auto bufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eDeviceLocal };

        vk::BufferCreateInfo vertexBufferInfo{ {}, vertexBufferSize, vertexBufferUsageFlags };
        m_vertexBuffer = static_cast<vk::Device>(m_device).createBufferUnique(vertexBufferInfo);

        vk::BufferCreateInfo indexBufferInfo{ {}, indexBufferSize, indexBufferUsageFlags };
        m_indexBuffer = static_cast<vk::Device>(m_device).createBufferUnique(indexBufferInfo);

        const auto vertexMemRequirements{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*m_vertexBuffer) };
        const auto indexMemRequirements{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*m_indexBuffer) };
        const auto vertexMemoryType{ m_instance.getPhysicalDevice().findMemoryType(vertexMemRequirements.memoryTypeBits, bufferMemoryPropertyFlags) };
        const auto indexMemoryType{ m_instance.getPhysicalDevice().findMemoryType(indexMemRequirements.memoryTypeBits, bufferMemoryPropertyFlags) };
        if (vertexMemoryType == indexMemoryType)
        {
            auto alignment = vertexMemRequirements.alignment;
            auto n = vertexBufferSize / alignment;
            m_combinedBufferOffset = (n + 1) * alignment;
            vk::MemoryAllocateInfo allocInfo{ vertexMemRequirements.size + indexMemRequirements.size, vertexMemoryType };
            m_combinedBufferMemory = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);
        }
        else
        {
            throw std::runtime_error("memory can't be combined!");
        }

        static_cast<vk::Device>(m_device).bindBufferMemory(*m_vertexBuffer, *m_combinedBufferMemory, 0);
        static_cast<vk::Device>(m_device).bindBufferMemory(*m_indexBuffer, *m_combinedBufferMemory, m_combinedBufferOffset);

        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        vertexStagingBuffer.buffer.copyToBuffer(cmdBuffer, m_vertexBuffer, vertexBufferSize);
        indexStagingBuffer.buffer.copyToBuffer(cmdBuffer, m_indexBuffer, indexBufferSize);
        /*stagingBuffer.buffer.copyToBuffer(cmdBuffer, m_vertexBuffer, vertexBufferSize);
        stagingBuffer.buffer.copyToBuffer(cmdBuffer, m_indexBuffer, indexBufferSize, vertexBufferSize);*/
        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_queue.waitIdle();
    }

    void CombinedBufferDemo::createUniformBuffer()
    {
        const auto bufferSize{ sizeof(UniformBufferObject) };
        const auto uniformBufferUsageFlags{ vk::BufferUsageFlagBits::eUniformBuffer };
        const auto uniformBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        createBuffer(bufferSize, uniformBufferUsageFlags, uniformBufferMemoryPropertyFlags, m_uniformBuffer, m_uniformBufferMemory);
    }

    void CombinedBufferDemo::createDescriptorPool()
    {
        vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
        vk::DescriptorPoolSize poolSizeTex{ vk::DescriptorType::eCombinedImageSampler, 1 };
        std::vector<vk::DescriptorPoolSize> vec{ poolSize, poolSizeTex };
        m_descriptorPool = m_device.createDescriptorPool(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, vec);
    }

    void CombinedBufferDemo::createDescriptorSet()
    {
        vk::DescriptorSetLayout layouts[] = { *m_descriptorSetLayout };
        vk::DescriptorSetAllocateInfo allocInfo{ *m_descriptorPool, 1, layouts };
        m_descriptorSets = static_cast<vk::Device>(m_device).allocateDescriptorSetsUnique(allocInfo);

        vk::DescriptorBufferInfo bufferInfo{ *m_uniformBuffer, 0, sizeof(UniformBufferObject) };
        const auto & sampler = reinterpret_cast<const vk::UniqueSampler &>(m_textureSampler);
        vk::DescriptorImageInfo imageInfo{ *sampler, *m_textureImageView, vk::ImageLayout::eShaderReadOnlyOptimal };
        WriteDescriptorSet descriptorWrite{ m_descriptorSets[0], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo };
        WriteDescriptorSet descriptorWriteTex{ m_descriptorSets[0], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo };
        std::vector<vk::WriteDescriptorSet> vec{ descriptorWrite, descriptorWriteTex };
        m_device.updateDescriptorSets(vec);
    }

    void CombinedBufferDemo::createCommandBuffers()
    {
        m_commandBuffers = m_device.allocateCommandBuffers(m_commandPool, static_cast<uint32_t>(m_swapChainFramebuffers.size()));
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            const auto & cmdBuffer{ m_commandBuffers[i] };
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            cmdBuffer.beginRenderPass(m_renderPass, m_swapChainFramebuffers[i], { { 0, 0 }, m_swapchain.getExtent() }, { ClearColorValue{ 0.f, 0.f, 0.f, 1.f } });
            cmdBuffer.bindPipeline(m_graphicsPipeline);
            cmdBuffer.bindDescriptorSet(m_pipelineLayout, m_descriptorSets[0]);
            cmdBuffer.bindVertexBuffer(m_vertexBuffer);
            cmdBuffer.bindIndexBuffer(m_indexBuffer, vk::IndexType::eUint16);
            cmdBuffer.drawIndexed(static_cast<uint32_t>(indices.size()));
            cmdBuffer.endRenderPass();
            cmdBuffer.end();
        }
    }

    void CombinedBufferDemo::updateUniformBuffer()
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

    void CombinedBufferDemo::drawFrame()
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
