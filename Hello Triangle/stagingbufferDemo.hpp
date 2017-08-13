#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "window.hpp"
#include "queue.hpp"
#include "swapchain.hpp"
#include "demo.hpp"

namespace bmvk
{
    class StagingbufferDemo : Demo
    {
    public:
        StagingbufferDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        StagingbufferDemo(const StagingbufferDemo &) = delete;
        StagingbufferDemo(StagingbufferDemo && other) = default;
        StagingbufferDemo & operator=(const StagingbufferDemo &) = delete;
        StagingbufferDemo & operator=(StagingbufferDemo && other) = default;
        ~StagingbufferDemo() {}

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
            { { 0.0f, -0.5f },{ 1.0f, 1.0f, 0.0f } },
            { { 0.5f, 0.5f },{ 0.0f, 1.0f, 0.0f } },
            { { -0.5f, 0.5f },{ 0.0f, 1.0f, 1.0f } }
        };

        Queue m_queue;
        Swapchain m_swapchain;
        vk::UniqueRenderPass m_renderPass;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipeline m_graphicsPipeline;
        std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;
        vk::UniqueCommandPool m_commandPool;
        vk::UniqueBuffer m_vertexBuffer;
        vk::UniqueDeviceMemory m_vertexBufferMemory;
        std::vector<vk::UniqueCommandBuffer> m_commandBuffers;
        vk::UniqueSemaphore m_imageAvailableSemaphore;
        vk::UniqueSemaphore m_renderFinishedSemaphore;

        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory);
        void copyBuffer(vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const;
        void createVertexBuffer();
        void createCommandBuffers();

        void drawFrame();
    };

    static_assert(std::is_move_constructible_v<StagingbufferDemo>);
    static_assert(std::is_move_assignable_v<StagingbufferDemo>);
    static_assert(!std::is_copy_constructible_v<StagingbufferDemo>);
    static_assert(!std::is_copy_assignable_v<StagingbufferDemo>);
}