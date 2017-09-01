#pragma once

#include <glm/glm.hpp>

#include "imguiBaseDemo.hpp"

namespace bmvk
{
    class TextureDemo : ImguiBaseDemo
    {
    public:
        TextureDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        TextureDemo(const TextureDemo &) = delete;
        TextureDemo(TextureDemo && other) = default;
        TextureDemo & operator=(const TextureDemo &) = delete;
        TextureDemo & operator=(TextureDemo && other) = default;
        ~TextureDemo() {}

        void run() override;
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
            { { -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f } },
            { { 0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f } },
            { { 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f } },
            { { -0.5f, 0.5f },{ 1.0f, 1.0f, 1.0f } }
        };

        std::vector<uint16_t> indices = {
            0, 1, 2, 2, 3, 0
        };

        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };

        vk::UniqueRenderPass m_renderPass;
        vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipeline m_graphicsPipeline;
        std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;
        vk::UniqueImage m_textureImage;
        vk::UniqueDeviceMemory m_textureImageMemory;
        vk::UniqueBuffer m_vertexBuffer;
        vk::UniqueDeviceMemory m_vertexBufferMemory;
        vk::UniqueBuffer m_indexBuffer;
        vk::UniqueDeviceMemory m_indexBufferMemory;
        vk::UniqueBuffer m_uniformBuffer;
        vk::UniqueDeviceMemory m_uniformBufferMemory;
        vk::UniqueDescriptorPool m_descriptorPool;
        std::vector<vk::UniqueDescriptorSet> m_descriptorSets;
        std::vector<CommandBuffer> m_commandBuffers;
        vk::UniqueSemaphore m_imageAvailableSemaphore;
        vk::UniqueSemaphore m_renderFinishedSemaphore;
        vk::UniqueSemaphore m_renderImguiFinishedSemaphore;

        void createDescriptorSetLayout();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createTextureImage();
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffer();
        void createDescriptorPool();
        void createDescriptorSet();
        void createCommandBuffers();

        void updateUniformBuffer();

        void drawFrame();
    };

    static_assert(std::is_move_constructible_v<TextureDemo>);
    static_assert(std::is_move_assignable_v<TextureDemo>);
    static_assert(!std::is_copy_constructible_v<TextureDemo>);
    static_assert(!std::is_copy_assignable_v<TextureDemo>);
}
