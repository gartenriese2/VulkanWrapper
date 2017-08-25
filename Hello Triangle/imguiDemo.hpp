#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "demo.hpp"
#include "swapchain.hpp"

struct ImDrawData;

namespace bmvk
{
    class ImguiDemo : Demo
    {
    public:
        ImguiDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height);
        ImguiDemo(const ImguiDemo &) = delete;
        ImguiDemo(ImguiDemo && other) = default;
        ImguiDemo & operator=(const ImguiDemo &) = delete;
        ImguiDemo & operator=(ImguiDemo && other) = default;
        ~ImguiDemo() {}

        void run() override;
        void recreateSwapChain();

        void imguiMouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
        void imguiScrollCallback(GLFWwindow * window, double xoffset, double yoffset);
        void imguiKeyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) const;
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

        Swapchain m_swapchain;
        vk::UniqueRenderPass m_renderPass;
        vk::UniqueRenderPass m_renderPassImgui;
        vk::UniqueSampler m_fontSamplerImgui;
        vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
        vk::UniqueDescriptorSetLayout m_descriptorSetLayoutImgui;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipelineLayout m_pipelineLayoutImgui;
        vk::UniquePipeline m_graphicsPipeline;
        vk::UniquePipeline m_graphicsPipelineImgui;
        std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;
        vk::UniqueBuffer m_vertexBuffer;
        vk::UniqueBuffer m_vertexBufferImgui;
        vk::UniqueDeviceMemory m_vertexBufferMemory;
        vk::UniqueDeviceMemory m_vertexBufferMemoryImgui;
        vk::UniqueBuffer m_indexBuffer;
        vk::UniqueBuffer m_indexBufferImgui;
        vk::UniqueDeviceMemory m_indexBufferMemory;
        vk::UniqueDeviceMemory m_indexBufferMemoryImgui;
        vk::UniqueBuffer m_uniformBuffer;
        vk::UniqueDeviceMemory m_uniformBufferMemory;
        vk::UniqueDescriptorPool m_descriptorPool;
        vk::UniqueDescriptorPool m_descriptorPoolImgui;
        std::vector<vk::UniqueDescriptorSet> m_descriptorSets;
        std::vector<vk::UniqueDescriptorSet> m_descriptorSetsImgui;
        std::vector<CommandBuffer> m_commandBuffers;
        std::unique_ptr<CommandBuffer> m_commandBufferImguiPtr;
        vk::UniqueSemaphore m_imageAvailableSemaphore;
        vk::UniqueSemaphore m_renderFinishedSemaphore;
        vk::UniqueSemaphore m_renderImguiFinishedSemaphore;
        vk::UniqueImage m_imguiFontImage;
        vk::UniqueDeviceMemory m_imguiFontMemory;
        vk::UniqueImageView m_imguiFontImageView;
        size_t m_bufferMemoryAlignmentImgui = 256;
        size_t m_vertexBufferSize;
        size_t m_indexBufferSize;

        double m_imguiTime = 0.0;
        std::vector<bool> m_imguiMousePressed = { false, false, false };
        float m_imguiMouseWheel = 0.f;

        void createRenderPass();
        void createFontSampler();
        void createDescriptorSetLayout();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffer();
        void createDescriptorPool();
        void createDescriptorSet();
        void createCommandBuffers();
        void uploadFonts();

        void drawFrame();
        void imguiRenderDrawLists(ImDrawData * draw_data);
        void drawImgui(uint32_t imageIndex);
        void updateUniformBuffer();
        void imguiNewFrame();

        
    };

    static_assert(std::is_move_constructible_v<ImguiDemo>);
    static_assert(std::is_move_assignable_v<ImguiDemo>);
    static_assert(!std::is_copy_constructible_v<ImguiDemo>);
    static_assert(!std::is_copy_assignable_v<ImguiDemo>);
}
