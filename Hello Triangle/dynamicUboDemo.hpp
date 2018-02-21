#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vw/model.hpp>

#include "imguiBaseDemo.hpp"

namespace bmvk
{
    class DynamicUboDemo : ImguiBaseDemo
    {
    public:
        DynamicUboDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        DynamicUboDemo(const DynamicUboDemo &) = delete;
        DynamicUboDemo(DynamicUboDemo && other) = default;
        DynamicUboDemo & operator=(const DynamicUboDemo &) = delete;
        DynamicUboDemo & operator=(DynamicUboDemo &&) = delete;
        virtual ~DynamicUboDemo();

        void run() override;
        void recreateSwapChain() override;
    private:
        static const uint32_t k_maxObjectInstances = 1000;

        glm::vec3 m_rotations[k_maxObjectInstances];
        glm::vec3 m_rotationSpeeds[k_maxObjectInstances];

        uint32_t m_objectInstances = 27;
        int m_numCubesI = 2;

        struct UniformBufferObject {
            glm::mat4 view;
            glm::mat4 proj;
        };

        struct DynamicUniformBufferObject {
            glm::mat4 * model = nullptr;
        } m_dynamicUniformBufferObject;

        size_t m_dynamicAlignment;
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

        vk::UniqueDeviceMemory m_dynamicUniformBufferMemory;
        vk::UniqueBuffer m_dynamicUniformBuffer;

        vk::UniqueDescriptorPool m_descriptorPool;
        std::vector<vk::UniqueDescriptorSet> m_descriptorSets;
        std::vector<CommandBuffer> m_commandBuffers;
        vk::UniqueSemaphore m_imageAvailableSemaphore;
        vk::UniqueSemaphore m_renderFinishedSemaphore;
        vk::UniqueSemaphore m_renderImguiFinishedSemaphore;

        vw::scene::Model<vw::scene::VertexDescription::PositionNormalColorTexture> m_cube;

        void setupCamera();

        void createDescriptorSetLayout();
        void createRenderPass();
        void createPipelines();
        void createDepthResources();
        void createFramebuffers();
        void loadCube();
        void createUniformBuffer();
        void createDynamicUniformBuffer();
        void createDescriptorPool();
        void createDescriptorSet();
        void createCommandBuffers();

        void updateNumObjects();
        void updateUniformBuffer();
        void updateDynamicUniformBuffer();

        void drawFrame();
    };

    static_assert(std::is_move_constructible_v<DynamicUboDemo>);
    static_assert(!std::is_copy_constructible_v<DynamicUboDemo>);
    static_assert(!std::is_move_assignable_v<DynamicUboDemo>);
    static_assert(!std::is_copy_assignable_v<DynamicUboDemo>);
}
#pragma once
