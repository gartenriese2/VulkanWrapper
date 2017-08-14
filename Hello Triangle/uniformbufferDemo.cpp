#include "uniformbufferDemo.hpp"

#include <glm/gtc/matrix_transform.hpp>

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

        auto app = reinterpret_cast<UniformbufferDemo *>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    UniformbufferDemo::UniformbufferDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
        : Demo{ enableValidationLayers, width, height, "Indexbuffer Demo" },
        m_swapchain{ m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window, m_device },
        m_imageAvailableSemaphore{ m_device.createSemaphore() },
        m_renderFinishedSemaphore{ m_device.createSemaphore() }
    {
        m_window.setWindowUserPointer(this);
        m_window.setWindowSizeCallback(onWindowResized);
        createRenderPass();
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

    void UniformbufferDemo::run()
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

    void UniformbufferDemo::recreateSwapChain()
    {
        m_device.waitIdle();

        for (auto & fb : m_swapChainFramebuffers)
        {
            fb.reset(nullptr);
        }

        for (auto & buffer : m_commandBuffers)
        {
            buffer.reset(nullptr);
        }

        m_graphicsPipeline.reset(nullptr);
        m_pipelineLayout.reset(nullptr);
        m_renderPass.reset(nullptr);
        m_swapchain.recreate(m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window, m_device);
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }

    void UniformbufferDemo::createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        vk::RenderPassCreateInfo renderPassInfo{ {}, 1, &colorAttachment, 1, &subpass, 1, &dependency };
        m_renderPass = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);
    }

    void UniformbufferDemo::createDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
        vk::DescriptorSetLayoutCreateInfo layoutInfo{ {}, 1, &uboLayoutBinding };
        m_descriptorSetLayout = static_cast<vk::Device>(m_device).createDescriptorSetLayoutUnique(layoutInfo);
    }

    void UniformbufferDemo::createGraphicsPipeline()
    {
        const auto vertShader{ Shader("../shaders/uniformbuffer.vert.spv", m_device) };
        const auto fragShader{ Shader("../shaders/uniformbuffer.frag.spv", m_device) };
        const auto vertShaderStageInfo{ vertShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex) };
        const auto fragShaderStageInfo{ fragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ {}, 1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data() };
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList };
        vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(m_swapchain.getExtent().width), static_cast<float>(m_swapchain.getExtent().height), 0.f, 1.f };
        vk::Rect2D scissor{ {}, m_swapchain.getExtent() };
        vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, &viewport, 1, &scissor };
        vk::PipelineRasterizationStateCreateInfo rasterizer{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, false, 0.f, 0.f, 0.f, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisampling;
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{ false, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlending{ {}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };
        auto descriptorSetLayout{ m_descriptorSetLayout.get() };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ {}, 1, &descriptorSetLayout };
        m_pipelineLayout = static_cast<vk::Device>(m_device).createPipelineLayoutUnique(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo{ {}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending, nullptr, m_pipelineLayout.get(), m_renderPass.get(), 0, nullptr, -1 };
        m_graphicsPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, pipelineInfo);
    }

    void UniformbufferDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.resize(m_swapchain.getImageViews().size());
        for (size_t i = 0; i < m_swapchain.getImageViews().size(); ++i)
        {
            vk::ImageView attachments[]{ m_swapchain.getImageViews()[i].get() };
            vk::FramebufferCreateInfo framebufferInfo{ {}, m_renderPass.get(), 1, attachments, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1 };
            m_swapChainFramebuffers[i] = static_cast<vk::Device>(m_device).createFramebufferUnique(framebufferInfo);
        }
    }

    void UniformbufferDemo::createVertexBuffer()
    {
        const auto bufferSize{ sizeof(vertices[0]) * vertices.size() };

        const auto stagingBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferSrc };
        const auto stagingBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, stagingBufferUsageFlags, stagingBufferMemoryPropertyFlags, stagingBuffer, stagingBufferMemory);

        auto data{ static_cast<vk::Device>(m_device).mapMemory(stagingBufferMemory.get(), 0, bufferSize) };
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        static_cast<vk::Device>(m_device).unmapMemory(stagingBufferMemory.get());

        const auto vertexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer };
        const auto vertexBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eDeviceLocal };
        createBuffer(bufferSize, vertexBufferUsageFlags, vertexBufferMemoryPropertyFlags, m_vertexBuffer, m_vertexBufferMemory);

        copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
    }

    void UniformbufferDemo::createIndexBuffer()
    {
        const auto bufferSize{ sizeof(indices[0]) * indices.size() };

        const auto stagingBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferSrc };
        const auto stagingBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, stagingBufferUsageFlags, stagingBufferMemoryPropertyFlags, stagingBuffer, stagingBufferMemory);

        auto data{ static_cast<vk::Device>(m_device).mapMemory(stagingBufferMemory.get(), 0, bufferSize) };
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        static_cast<vk::Device>(m_device).unmapMemory(stagingBufferMemory.get());

        const auto indexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer };
        const auto indexBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eDeviceLocal };
        createBuffer(bufferSize, indexBufferUsageFlags, indexBufferMemoryPropertyFlags, m_indexBuffer, m_indexBufferMemory);

        copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
    }

    void UniformbufferDemo::createUniformBuffer()
    {
        const auto bufferSize{ sizeof(UniformBufferObject) };
        const auto uniformBufferUsageFlags{ vk::BufferUsageFlagBits::eUniformBuffer };
        const auto uniformBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        createBuffer(bufferSize, uniformBufferUsageFlags, uniformBufferMemoryPropertyFlags, m_uniformBuffer, m_uniformBufferMemory);
    }

    void UniformbufferDemo::createDescriptorPool()
    {
        vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
        vk::DescriptorPoolCreateInfo poolInfo{ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, 1, &poolSize };
        m_descriptorPool = static_cast<vk::Device>(m_device).createDescriptorPoolUnique(poolInfo);
    }

    void UniformbufferDemo::createDescriptorSet()
    {
        vk::DescriptorSetLayout layouts[] = { m_descriptorSetLayout.get() };
        vk::DescriptorSetAllocateInfo allocInfo{ m_descriptorPool.get(), 1, layouts };
        m_descriptorSets = static_cast<vk::Device>(m_device).allocateDescriptorSetsUnique(allocInfo);

        vk::DescriptorBufferInfo bufferInfo{ m_uniformBuffer.get(), 0, sizeof(UniformBufferObject) };
        auto descriptorSet{ m_descriptorSets[0].get() };
        vk::WriteDescriptorSet descriptorWrite{ descriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo };
        static_cast<vk::Device>(m_device).updateDescriptorSets(descriptorWrite, nullptr);
    }

    void UniformbufferDemo::createCommandBuffers()
    {
        m_commandBuffers.resize(m_swapChainFramebuffers.size());
        CommandBufferAllocateInfo allocInfo{ m_commandPool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(m_commandBuffers.size()) };
        m_commandBuffers = static_cast<vk::Device>(m_device).allocateCommandBuffersUnique(allocInfo);
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            m_commandBuffers[i].get().begin(beginInfo);
            vk::ClearValue clearColor{ vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }) };
            vk::RenderPassBeginInfo renderPassInfo{ m_renderPass.get(), m_swapChainFramebuffers[i].get(), vk::Rect2D({ 0, 0 }, m_swapchain.getExtent()), 1, &clearColor };
            m_commandBuffers[i].get().beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
            m_commandBuffers[i].get().bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.get());
            m_commandBuffers[i].get().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, m_descriptorSets[0].get(), nullptr);
            m_commandBuffers[i].get().bindVertexBuffers(0, m_vertexBuffer.get(), {0});
            m_commandBuffers[i].get().bindIndexBuffer(m_indexBuffer.get(), 0, vk::IndexType::eUint16);
            m_commandBuffers[i].get().drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            m_commandBuffers[i].get().endRenderPass();
            m_commandBuffers[i].get().end();
        }
    }

    void UniformbufferDemo::drawFrame()
    {
        m_queue.waitIdle();
        timing();

        uint32_t imageIndex;
        try
        {
            static_cast<vk::Device>(m_device).acquireNextImageKHR(static_cast<vk::SwapchainKHR>(m_swapchain), std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphore.get(), nullptr, &imageIndex);
        }
        catch (const vk::OutOfDateKHRError &)
        {
            recreateSwapChain();
            return;
        }

        vk::Semaphore waitSemaphores[]{ m_imageAvailableSemaphore.get() };
        vk::PipelineStageFlags waitStages[]{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
        auto usedCommandBuffer = m_commandBuffers[imageIndex].get();
        vk::Semaphore signalSemaphores[]{ m_renderFinishedSemaphore.get() };
        vk::SubmitInfo submitInfo{ 1, waitSemaphores, waitStages, 1, &usedCommandBuffer, 1, signalSemaphores };
        static_cast<vk::Queue>(m_queue).submit(submitInfo, nullptr);
        vk::SwapchainKHR swapchains[]{ static_cast<vk::SwapchainKHR>(m_swapchain) };
        vk::PresentInfoKHR presentInfo{ 1, signalSemaphores, 1, swapchains, &imageIndex };
        vk::Result result;
        auto notOutOfDate{ false };
        try
        {
            result = static_cast<vk::Queue>(m_queue).presentKHR(presentInfo);
            notOutOfDate = true;
        }
        catch (const vk::OutOfDateKHRError &)
        {
            recreateSwapChain();
        }

        if (notOutOfDate && result == vk::Result::eSuboptimalKHR)
        {
            recreateSwapChain();
        }
    }

    void UniformbufferDemo::updateUniformBuffer()
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

        UniformBufferObject ubo;
        ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.f), m_swapchain.getExtent().width / static_cast<float>(m_swapchain.getExtent().height), 0.1f, 10.f);
        ubo.proj[1][1] *= -1;

        auto data{ static_cast<vk::Device>(m_device).mapMemory(m_uniformBufferMemory.get(), 0, sizeof(ubo)) };
        memcpy(data, &ubo, sizeof(ubo));
        static_cast<vk::Device>(m_device).unmapMemory(m_uniformBufferMemory.get());
    }
}
