#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "imguiBaseDemo.hpp"

namespace bmvk
{
    class DragonDemo : ImguiBaseDemo
    {
    public:
        DragonDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        DragonDemo(const DragonDemo &) = delete;
        DragonDemo(DragonDemo && other) = default;
        DragonDemo & operator=(const DragonDemo &) = delete;
        DragonDemo & operator=(DragonDemo &&) = delete;

        void run() override;
        void recreateSwapChain() override;
    private:
        struct Vertex
        {
            glm::tvec3<float> pos;
            glm::tvec3<float> color;

            static vk::VertexInputBindingDescription getBindingDescription()
            {
                return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
            }

            static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
            {
                std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;
                attributeDescriptions[0] = vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) };
                attributeDescriptions[1] = vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) };
                return attributeDescriptions;
            }
        };

        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };

        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;

        vk::UniqueRenderPass m_renderPass;
        vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipeline m_graphicsPipeline;
        std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;

        vk::UniqueImage m_depthImage;
        vk::UniqueDeviceMemory m_depthImageMemory;
        vk::UniqueImageView m_depthImageView;

        vk::UniqueBuffer m_vertexBuffer;
        vk::UniqueBuffer m_indexBuffer;
        vk::UniqueDeviceMemory m_combinedBufferMemory;
        vk::DeviceSize m_combinedBufferOffset;

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
        void createDepthResources();
        void createFramebuffers();
        void loadModelWithAssimp();
        void loadModel();
        void createCombinedBuffer();
        void createUniformBuffer();
        void createDescriptorPool();
        void createDescriptorSet();
        void createCommandBuffers();

        void updateUniformBuffer();

        void drawFrame();
    };

    static_assert(std::is_move_constructible_v<DragonDemo>);
    static_assert(!std::is_copy_constructible_v<DragonDemo>);
    static_assert(!std::is_move_assignable_v<DragonDemo>);
    static_assert(!std::is_copy_assignable_v<DragonDemo>);
}
