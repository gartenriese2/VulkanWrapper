#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vw/modelRepository.hpp>

#include "imguiBaseDemo.hpp"

namespace bmvk
{
    class ModelRepositoryDemo : ImguiBaseDemo<vw::scene::VertexDescription::PositionNormalColor>
    {
    public:
        ModelRepositoryDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        ModelRepositoryDemo(const ModelRepositoryDemo &) = delete;
        ModelRepositoryDemo(ModelRepositoryDemo && other) = default;
        ModelRepositoryDemo & operator=(const ModelRepositoryDemo &) = delete;
        ModelRepositoryDemo & operator=(ModelRepositoryDemo &&) = default;

        void run() override;
        void recreateSwapChain() override;
    private:
        static const uint32_t k_maxObjectInstances = 1000;

        glm::vec3 m_rotations[k_maxObjectInstances];
        glm::vec3 m_rotationSpeeds[k_maxObjectInstances];

        std::vector<vw::scene::ModelID> m_modelIDs;

        int m_numCubesI = 2;
        uint32_t m_currentNumInstances = 8;

        struct UniformBufferObject {
            glm::mat4 view;
            glm::mat4 proj;
        };

        float m_animationTimer = 0.0f;
        std::vector<double> m_lastFrameTimes;
        double m_avgRenderFrameTime = 0.0;
        double m_avgImguiRenderFrameTime = 0.0;
        vk::UniqueQueryPool m_queryPool;

        vk::UniqueRenderPass m_renderPass;
        vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipeline m_pipeline;
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

        vw::scene::ModelResourceID m_cubeResourceId;

        void setupCamera();
        void initModels();

        void createDescriptorSetLayout();
        void createRenderPass();
        void createPipelines();
        void createDepthResources();
        void createFramebuffers();
        
        void createUniformBuffer();
        void createRotations();
        void createDescriptorPool();
        void createDescriptorSet();
        void createCommandBuffers();

        void updateNumObjects();
        void updateUniformBuffer();
        void updateDynamicUniformBuffer();

        void drawFrame();
    };
}
