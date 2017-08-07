#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

#include "window.hpp"
#include "instance.hpp"
#include "device.hpp"
#include "queue.hpp"
#include "swapchain.hpp"
#include <glm/glm.hpp>

namespace bmvk
{
    class VertexbufferDemo
    {
    public:
        VertexbufferDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        VertexbufferDemo(const VertexbufferDemo &) = delete;
        VertexbufferDemo(VertexbufferDemo && other) = default;
        VertexbufferDemo & operator=(const VertexbufferDemo &) = delete;
        VertexbufferDemo & operator=(VertexbufferDemo && other) = default;
        ~VertexbufferDemo() {}

        void run();
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

        Window m_window;
        Instance m_instance;
        Device m_device;
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
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
        void createVertexBuffer();
        void createCommandBuffers();

        void drawFrame();
    };

    static_assert(std::is_move_constructible_v<VertexbufferDemo>);
    static_assert(std::is_move_assignable_v<VertexbufferDemo>);
    static_assert(!std::is_copy_constructible_v<VertexbufferDemo>);
    static_assert(!std::is_copy_assignable_v<VertexbufferDemo>);
}
