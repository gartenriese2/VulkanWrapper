#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vw/model.hpp>

#include "imguiBaseDemo.hpp"

namespace bmvk
{
    template <vw::scene::VertexDescription VD>
    class CoordinatesDemo : ImguiBaseDemo<VD>
    {
    public:
        CoordinatesDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        CoordinatesDemo(const CoordinatesDemo &) = delete;
        CoordinatesDemo(CoordinatesDemo && other) = default;
        CoordinatesDemo & operator=(const CoordinatesDemo &) = delete;
        CoordinatesDemo & operator=(CoordinatesDemo &&) = default;

        void run() override;
        void recreateSwapChain() override;
    private:
        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
            glm::mat4 normal;
        };

        vk::UniqueRenderPass m_renderPass;
        vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipeline m_colorPipeline;
        vk::UniquePipeline m_normalPipeline;
        vk::UniquePipeline m_worldNormalPipeline;
        vk::UniquePipeline m_viewPosPipeline;
        std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;

        vk::UniqueDeviceMemory m_depthImageMemory;
        vk::UniqueImage m_depthImage;
        vk::UniqueImageView m_depthImageView;

        vk::UniqueDeviceMemory m_uniformBufferMemory;
        vk::UniqueBuffer m_uniformBuffer;

        vk::UniqueDescriptorPool m_descriptorPool;
        std::vector<vk::UniqueDescriptorSet> m_descriptorSets;
        std::vector<CommandBuffer> m_commandBuffers;
        vk::UniqueSemaphore m_imageAvailableSemaphore;
        vk::UniqueSemaphore m_renderFinishedSemaphore;
        vk::UniqueSemaphore m_renderImguiFinishedSemaphore;

        vw::scene::Model<VD> m_cube;
        vw::scene::Model<VD> m_dragon;

        void setupCamera();

        void createDescriptorSetLayout();
        void createRenderPass();
        void createPipelines();
        void createDepthResources();
        void createFramebuffers();
        void loadCube();
        void loadDragon();
        void createUniformBuffer();
        void createDescriptorPool();
        void createDescriptorSet();
        void createCommandBuffers();

        void updateUniformBuffer();

        void drawFrame();
    };
}
