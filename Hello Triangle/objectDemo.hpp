#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "imguiBaseDemo.hpp"

namespace bmvk
{
    template <vw::scene::VertexDescription VD>
    class ObjectDemo : ImguiBaseDemo<VD>
    {
    public:
        ObjectDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        ObjectDemo(const ObjectDemo &) = delete;
        ObjectDemo(ObjectDemo && other) = default;
        ObjectDemo & operator=(const ObjectDemo &) = delete;
        ObjectDemo & operator=(ObjectDemo &&) = default;

        void run() override;
        void recreateSwapChain() override;
    private:
        struct Vertex
        {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec2 texCoord;

            static vk::VertexInputBindingDescription getBindingDescription()
            {
                return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
            }

            static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
            {
                std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;
                attributeDescriptions[0] = vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) };
                attributeDescriptions[1] = vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) };
                attributeDescriptions[2] = vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) };
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

        vk::UniqueImage m_textureImage;
        vk::UniqueDeviceMemory m_textureImageMemory;
        vk::UniqueImageView m_textureImageView;
        Sampler m_textureSampler;

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
        void createTextureImage();
        void createTextureImageView();
        void loadModel();
        void createCombinedBuffer();
        void createUniformBuffer();
        void createDescriptorPool();
        void createDescriptorSet();
        void createCommandBuffers();

        void updateUniformBuffer();

        void drawFrame();
    };
}
