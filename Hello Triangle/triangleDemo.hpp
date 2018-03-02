#pragma once

#include "demo.hpp"
#include "swapchain.hpp"

namespace bmvk
{
    template <vw::scene::VertexDescription VD>
    class TriangleDemo : Demo<VD>
    {
    public:
        explicit TriangleDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        TriangleDemo(const TriangleDemo &) = delete;
        TriangleDemo(TriangleDemo && other) = default;
        TriangleDemo & operator=(const TriangleDemo &) = delete;
        TriangleDemo & operator=(TriangleDemo &&) = default;
        ~TriangleDemo() {}

        void run() override;
        void recreateSwapChain();
    private:
        Swapchain m_swapchain;
        vk::UniqueRenderPass m_renderPass;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipeline m_graphicsPipeline;
        std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;
        std::vector<vk::UniqueCommandBuffer> m_commandBuffers;
        vk::UniqueSemaphore m_imageAvailableSemaphore;
        vk::UniqueSemaphore m_renderFinishedSemaphore;

        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandBuffers();
        
        void drawFrame();
    };
}
