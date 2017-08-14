#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

#include "window.hpp"
#include "swapchain.hpp"
#include "demo.hpp"

namespace bmvk
{
    class TriangleDemo : Demo
    {
    public:
        explicit TriangleDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        TriangleDemo(const TriangleDemo &) = delete;
        TriangleDemo(TriangleDemo && other) = default;
        TriangleDemo & operator=(const TriangleDemo &) = delete;
        TriangleDemo & operator=(TriangleDemo && other) = default;
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

    static_assert(std::is_move_constructible_v<TriangleDemo>);
    static_assert(std::is_move_assignable_v<TriangleDemo>);
    static_assert(!std::is_copy_constructible_v<TriangleDemo>);
    static_assert(!std::is_copy_assignable_v<TriangleDemo>);
}
