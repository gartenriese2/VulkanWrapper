#include "vertexbufferDemo.hpp"

#include "shader.hpp"

namespace bmvk
{
    static void onWindowResized(GLFWwindow * window, int width, int height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        auto app = reinterpret_cast<VertexbufferDemo *>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    VertexbufferDemo::VertexbufferDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
      : Demo{ enableValidationLayers, width, height, "Vertexbuffer Demo" },
        m_swapchain{ m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window, m_device },
        m_imageAvailableSemaphore{ m_device.createSemaphore() },
        m_renderFinishedSemaphore{ m_device.createSemaphore() }
    {
        m_window.setWindowUserPointer(this);
        m_window.setWindowSizeCallback(onWindowResized);
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createVertexBuffer();
        createCommandBuffers();
    }

    void VertexbufferDemo::run()
    {
        while (!m_window.shouldClose())
        {
            m_window.pollEvents();
            // do cpu work here
            drawFrame();
        }

        m_device.waitIdle();
    }

    void VertexbufferDemo::recreateSwapChain()
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

    void VertexbufferDemo::createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ vk::AttachmentDescriptionFlags(), m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::SubpassDescription subpass{ vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlags(), vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        vk::RenderPassCreateInfo renderPassInfo{ vk::RenderPassCreateFlags(), 1, &colorAttachment, 1, &subpass, 1, &dependency };
        m_renderPass = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);
    }

    void VertexbufferDemo::createGraphicsPipeline()
    {
        const auto vertShader{ Shader("../shaders/vertexbuffer.vert.spv", m_device) };
        const auto fragShader{ Shader("../shaders/vertexbuffer.frag.spv", m_device) };
        const auto vertShaderStageInfo{ vertShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex) };
        const auto fragShaderStageInfo{ fragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ vk::PipelineVertexInputStateCreateFlags(), 1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data() };
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList };
        vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(m_swapchain.getExtent().width), static_cast<float>(m_swapchain.getExtent().height), 0.f, 1.f };
        vk::Rect2D scissor{ vk::Offset2D(), m_swapchain.getExtent() };
        vk::PipelineViewportStateCreateInfo viewportState{ vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor };
        vk::PipelineRasterizationStateCreateInfo rasterizer{ vk::PipelineRasterizationStateCreateFlags(), false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.f, 0.f, 0.f, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisampling;
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{ false, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlending{ vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        m_pipelineLayout = static_cast<vk::Device>(m_device).createPipelineLayoutUnique(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo{ vk::PipelineCreateFlags(), 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending, nullptr, m_pipelineLayout.get(), m_renderPass.get(), 0, nullptr, -1 };
        m_graphicsPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, pipelineInfo);
    }

    void VertexbufferDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.resize(m_swapchain.getImageViews().size());
        for (size_t i = 0; i < m_swapchain.getImageViews().size(); ++i)
        {
            vk::ImageView attachments[]{ m_swapchain.getImageViews()[i].get() };
            vk::FramebufferCreateInfo framebufferInfo{ vk::FramebufferCreateFlags(), m_renderPass.get(), 1, attachments, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1 };
            m_swapChainFramebuffers[i] = static_cast<vk::Device>(m_device).createFramebufferUnique(framebufferInfo);
        }
    }

    void VertexbufferDemo::createVertexBuffer()
    {
        vk::BufferCreateInfo bufferInfo{ vk::BufferCreateFlags(), sizeof(vertices[0]) * vertices.size(), vk::BufferUsageFlagBits::eVertexBuffer };
        m_vertexBuffer = static_cast<vk::Device>(m_device).createBufferUnique(bufferInfo);

        const auto memRequirements{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(m_vertexBuffer.get()) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, m_instance.getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };
        m_vertexBufferMemory = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);

        static_cast<vk::Device>(m_device).bindBufferMemory(m_vertexBuffer.get(), m_vertexBufferMemory.get(), 0);

        auto data{ static_cast<vk::Device>(m_device).mapMemory(m_vertexBufferMemory.get(), 0, bufferInfo.size) };
        memcpy(data, vertices.data(), static_cast<size_t>(bufferInfo.size));
        static_cast<vk::Device>(m_device).unmapMemory(m_vertexBufferMemory.get());
    }

    void VertexbufferDemo::createCommandBuffers()
    {
        m_commandBuffers.resize(m_swapChainFramebuffers.size());
        vk::CommandBufferAllocateInfo allocInfo{ m_commandPool.get(), vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(m_commandBuffers.size()) };
        m_commandBuffers = static_cast<vk::Device>(m_device).allocateCommandBuffersUnique(allocInfo);
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            m_commandBuffers[i].get().begin(beginInfo);
            vk::ClearValue clearColor{ vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }) };
            vk::RenderPassBeginInfo renderPassInfo{ m_renderPass.get(), m_swapChainFramebuffers[i].get(), vk::Rect2D({ 0, 0 }, m_swapchain.getExtent()), 1, &clearColor };
            m_commandBuffers[i].get().beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
            m_commandBuffers[i].get().bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.get());

            vk::Buffer vertexBuffers[] = { m_vertexBuffer.get() };
            vk::DeviceSize offsets[] = { 0 };
            m_commandBuffers[i].get().bindVertexBuffers(0, 1, vertexBuffers, offsets);
            m_commandBuffers[i].get().draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            m_commandBuffers[i].get().endRenderPass();
            m_commandBuffers[i].get().end();
        }
    }

    void VertexbufferDemo::drawFrame()
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
        static_cast<vk::Queue>(m_queue).submit(1, &submitInfo, nullptr);
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
}
