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
    class PushConstantDemo : ImguiBaseDemo<VD>
    {
    public:
        PushConstantDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        PushConstantDemo(const PushConstantDemo &) = delete;
        PushConstantDemo(PushConstantDemo && other) = default;
        PushConstantDemo & operator=(const PushConstantDemo &) = delete;
        PushConstantDemo & operator=(PushConstantDemo &&) = default;

        void run() override;
        void recreateSwapChain() override;
    private:
        std::vector<glm::vec4> m_lightPositions;
        float m_shininess = 64.f;
        float m_ambientWhite = 0.01f;
        float m_screenGamma = 2.2f;
        float m_maxLightDist = 9.f;

        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
            glm::mat4 normal;
        };

        float m_animationTimer = 0.0f;

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

        vw::scene::Model<VD> m_model;

        void setupCamera();

        void createDescriptorSetLayout();
        void createRenderPass();
        void createPipelines();
        void createDepthResources();
        void createFramebuffers();
        void loadScene();
        void createUniformBuffer();
        void createDescriptorPool();
        void createDescriptorSet();
        void createCommandBuffers();
        void recreateCommandBuffers();
        void updateUniformBuffer();

        void drawFrame();
    };
}
