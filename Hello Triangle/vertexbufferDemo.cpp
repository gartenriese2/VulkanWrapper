#include "vertexbufferDemo.hpp"

#include "shader.hpp"
#include "vulkan_bmvk.hpp"

namespace bmvk
{
    template <vw::scene::VertexDescription VD>
    VertexbufferDemo<VD>::VertexbufferDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
      : Demo<VD>{ enableValidationLayers, width, height, "Vertexbuffer Demo", DebugReport::ReportLevel::WarningsAndAbove },
        m_swapchain{ m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window.getSize(), m_device },
        m_imageAvailableSemaphore{ m_device.createSemaphore() },
        m_renderFinishedSemaphore{ m_device.createSemaphore() }
    {
        m_window.addWindowSizeFunc([this](int width, int height)
        {
            recreateSwapChain();
        });
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createVertexBuffer();
        createCommandBuffers();
    }

    template <vw::scene::VertexDescription VD>
    void VertexbufferDemo<VD>::run()
    {
        while (!m_window.shouldClose())
        {
            m_window.pollEvents();
            // do cpu work here
            drawFrame();
        }

        m_device.waitIdle();
    }

    template <vw::scene::VertexDescription VD>
    void VertexbufferDemo<VD>::recreateSwapChain()
    {
        const auto[width, height] = m_window.getSize();
        if (width == 0 || height == 0)
        {
            return;
        }

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
        m_swapchain.recreate(m_instance.getPhysicalDevice(), m_instance.getSurface(), m_window.getSize(), m_device);
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }

    template <vw::scene::VertexDescription VD>
    void VertexbufferDemo<VD>::createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        vk::RenderPassCreateInfo renderPassInfo{ {}, 1, &colorAttachment, 1, &subpass, 1, &dependency };
        m_renderPass = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createRenderPassUnique(renderPassInfo);
    }

    template <vw::scene::VertexDescription VD>
    void VertexbufferDemo<VD>::createGraphicsPipeline()
    {
        const auto vertShader{ Shader("../shaders/vertexbuffer.vert.spv", m_device) };
        const auto fragShader{ Shader("../shaders/vertexbuffer.frag.spv", m_device) };
        const auto vertShaderStageInfo{ vertShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex) };
        const auto fragShaderStageInfo{ fragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ {}, 1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data() };
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList };
        vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(m_swapchain.getExtent().width), static_cast<float>(m_swapchain.getExtent().height), 0.f, 1.f };
        vk::Rect2D scissor{ vk::Offset2D(), m_swapchain.getExtent() };
        vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, &viewport, 1, &scissor };
        vk::PipelineRasterizationStateCreateInfo rasterizer{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.f, 0.f, 0.f, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisampling;
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{ false, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlending{ {}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        m_pipelineLayout = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createPipelineLayoutUnique(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo{ {}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending, nullptr, *m_pipelineLayout, *m_renderPass, 0, nullptr, -1 };
        m_graphicsPipeline = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createGraphicsPipelineUnique(nullptr, pipelineInfo);
    }

    template <vw::scene::VertexDescription VD>
    void VertexbufferDemo<VD>::createFramebuffers()
    {
        m_swapChainFramebuffers.resize(m_swapchain.getImageViews().size());
        for (size_t i = 0; i < m_swapchain.getImageViews().size(); ++i)
        {
            vk::ImageView attachments[]{ *m_swapchain.getImageViews()[i] };
            vk::FramebufferCreateInfo framebufferInfo{ {}, *m_renderPass, 1, attachments, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1 };
            m_swapChainFramebuffers[i] = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createFramebufferUnique(framebufferInfo);
        }
    }

    template <vw::scene::VertexDescription VD>
    void VertexbufferDemo<VD>::createVertexBuffer()
    {
        vk::BufferCreateInfo bufferInfo{ {}, sizeof(vertices[0]) * vertices.size(), vk::BufferUsageFlagBits::eVertexBuffer };
        m_vertexBuffer = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createBufferUnique(bufferInfo);

        const auto memRequirements{ reinterpret_cast<const vk::UniqueDevice &>(m_device)->getBufferMemoryRequirements(*m_vertexBuffer) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, m_instance.getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };
        m_vertexBufferMemory = reinterpret_cast<const vk::UniqueDevice &>(m_device)->allocateMemoryUnique(allocInfo);

        reinterpret_cast<const vk::UniqueDevice &>(m_device)->bindBufferMemory(*m_vertexBuffer, *m_vertexBufferMemory, 0);

        auto data{ reinterpret_cast<const vk::UniqueDevice &>(m_device)->mapMemory(*m_vertexBufferMemory, 0, bufferInfo.size) };
        memcpy(data, vertices.data(), static_cast<size_t>(bufferInfo.size));
        reinterpret_cast<const vk::UniqueDevice &>(m_device)->unmapMemory(*m_vertexBufferMemory);
    }

    template <vw::scene::VertexDescription VD>
    void VertexbufferDemo<VD>::createCommandBuffers()
    {
        m_commandBuffers.resize(m_swapChainFramebuffers.size());
        CommandBufferAllocateInfo allocInfo{ m_commandPool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(m_commandBuffers.size()) };
        m_commandBuffers = reinterpret_cast<const vk::UniqueDevice &>(m_device)->allocateCommandBuffersUnique(allocInfo);
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            (*m_commandBuffers[i]).begin(beginInfo);
            vk::ClearValue clearColor{ ClearColorValue{ 0.f, 0.f, 0.f, 1.f } };
            vk::RenderPassBeginInfo renderPassInfo{ *m_renderPass, *m_swapChainFramebuffers[i], vk::Rect2D({ 0, 0 }, m_swapchain.getExtent()), 1, &clearColor };
            (*m_commandBuffers[i]).beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
            (*m_commandBuffers[i]).bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);
            (*m_commandBuffers[i]).bindVertexBuffers(0, *m_vertexBuffer, {0});
            (*m_commandBuffers[i]).draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            (*m_commandBuffers[i]).endRenderPass();
            (*m_commandBuffers[i]).end();
        }
    }

    template <vw::scene::VertexDescription VD>
    void VertexbufferDemo<VD>::drawFrame()
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

        vk::Semaphore waitSemaphores[]{ *m_imageAvailableSemaphore };
        vk::PipelineStageFlags waitStages[]{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
        auto usedCommandBuffer = *m_commandBuffers[imageIndex];
        vk::Semaphore signalSemaphores[]{ *m_renderFinishedSemaphore };
        vk::SubmitInfo submitInfo{ 1, waitSemaphores, waitStages, 1, &usedCommandBuffer, 1, signalSemaphores };
        static_cast<vk::Queue>(m_queue).submit(submitInfo, nullptr);
        vk::SwapchainKHR swapchains[]{ *reinterpret_cast<const vk::UniqueSwapchainKHR &>(m_swapchain) };
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

    template class VertexbufferDemo<vw::scene::VertexDescription::NotUsed>;
}
