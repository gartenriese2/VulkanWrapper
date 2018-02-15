#pragma once

#include "demo.hpp"
#include "swapchain.hpp"

struct ImDrawData;

namespace bmvk
{
    class ImguiBaseDemo : protected Demo
    {
    public:
        ImguiBaseDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name, const bool onlyWarningsAndAbove);
        ImguiBaseDemo(const ImguiBaseDemo &) = delete;
        ImguiBaseDemo(ImguiBaseDemo && other) = default;
        ImguiBaseDemo & operator=(const ImguiBaseDemo &) = delete;
        ImguiBaseDemo & operator=(ImguiBaseDemo &&) = delete;
        ~ImguiBaseDemo() {}

        virtual void recreateSwapChain();

        void imguiMouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
        void imguiScrollCallback(GLFWwindow * window, double xoffset, double yoffset);
        void imguiKeyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) const;
    protected:
        Swapchain m_swapchain;

        void drawFrame(uint32_t imageIndex, const vk::UniqueSemaphore & renderFinishedSemaphore, const vk::UniqueSemaphore & renderImguiFinishedSemaphore);
        void imguiNewFrame();
    private:
        vk::UniqueRenderPass m_renderPassImgui;
        Sampler m_fontSampler;
        vk::UniqueDescriptorSetLayout m_descriptorSetLayoutImgui;
        vk::UniquePipelineLayout m_pipelineLayoutImgui;
        vk::UniquePipeline m_graphicsPipelineImgui;
        std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;
        vk::UniqueDeviceMemory m_vertexBufferMemoryImgui;
        vk::UniqueBuffer m_vertexBufferImgui;
        vk::UniqueDeviceMemory m_indexBufferMemoryImgui;
        vk::UniqueBuffer m_indexBufferImgui;
        vk::UniqueDescriptorPool m_descriptorPoolImgui;
        std::vector<vk::UniqueDescriptorSet> m_descriptorSetsImgui;
        std::unique_ptr<CommandBuffer> m_commandBufferImguiPtr;
        vk::UniqueDeviceMemory m_imguiFontMemory;
        vk::UniqueImage m_imguiFontImage;
        vk::UniqueImageView m_imguiFontImageView;
        size_t m_bufferMemoryAlignmentImgui = 256;
        size_t m_vertexBufferSize;
        size_t m_indexBufferSize;

        double m_imguiTime = 0.0;
        std::vector<bool> m_imguiMousePressed = { false, false, false };
        float m_imguiMouseWheel = 0.f;

        void createDescriptorSetLayout();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createDescriptorPool();
        void createDescriptorSet();
        void uploadFonts();

        void imguiRenderDrawLists(ImDrawData * draw_data);
    };

    static_assert(std::is_move_constructible_v<ImguiBaseDemo>);
    static_assert(!std::is_move_assignable_v<ImguiBaseDemo>);
    static_assert(!std::is_copy_constructible_v<ImguiBaseDemo>);
    static_assert(!std::is_copy_assignable_v<ImguiBaseDemo>);
}
