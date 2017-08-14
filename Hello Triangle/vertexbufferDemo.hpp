#pragma once

#include <glm/glm.hpp>

#include "demo.hpp"
#include "swapchain.hpp"

namespace bmvk
{
    class VertexbufferDemo : Demo
    {
    public:
        VertexbufferDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        VertexbufferDemo(const VertexbufferDemo &) = delete;
        VertexbufferDemo(VertexbufferDemo && other) = default;
        VertexbufferDemo & operator=(const VertexbufferDemo &) = delete;
        VertexbufferDemo & operator=(VertexbufferDemo && other) = default;
        ~VertexbufferDemo() {}

        void run() override;
        void recreateSwapChain();
    private:
        struct Vertex
        {
            glm::vec2 pos;
            glm::vec3 color;

            static vk::VertexInputBindingDescription getBindingDescription()
            {
                return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
            }

            static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
            {
                std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;
                attributeDescriptions[0] = vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos) };
                attributeDescriptions[1] = vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) };
                return attributeDescriptions;
            }
        };

        std::vector<Vertex> vertices = {
            { { 0.0f, -0.5f },{ 1.0f, 1.0f, 1.0f } },
            { { 0.5f, 0.5f },{ 0.0f, 1.0f, 0.0f } },
            { { -0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f } }
        };

        Swapchain m_swapchain;
        vk::UniqueRenderPass m_renderPass;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipeline m_graphicsPipeline;
        std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;
        vk::UniqueBuffer m_vertexBuffer;
        vk::UniqueDeviceMemory m_vertexBufferMemory;
        std::vector<vk::UniqueCommandBuffer> m_commandBuffers;
        vk::UniqueSemaphore m_imageAvailableSemaphore;
        vk::UniqueSemaphore m_renderFinishedSemaphore;

        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createVertexBuffer();
        void createCommandBuffers();

        void drawFrame();
    };

    static_assert(std::is_move_constructible_v<VertexbufferDemo>);
    static_assert(std::is_move_assignable_v<VertexbufferDemo>);
    static_assert(!std::is_copy_constructible_v<VertexbufferDemo>);
    static_assert(!std::is_copy_assignable_v<VertexbufferDemo>);
}
