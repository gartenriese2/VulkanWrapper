#pragma once

#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class CommandBuffer
    {
    public:
        explicit CommandBuffer(vk::UniqueCommandBuffer && commandBuffer) : m_commandBuffer{std::move(commandBuffer)} {}
        CommandBuffer(const CommandBuffer &) = delete;
        CommandBuffer(CommandBuffer && other) = default;
        CommandBuffer & operator=(const CommandBuffer &) = delete;
        CommandBuffer & operator=(CommandBuffer && other) = default;
        ~CommandBuffer() {}

        explicit operator const vk::UniqueCommandBuffer &() const noexcept { return m_commandBuffer; }

        void begin(vk::CommandBufferUsageFlags flags = {}) const;
        void end() const;

        void copyBuffer(vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const;
        void beginRenderPass(const vk::UniqueRenderPass & renderPass, const vk::UniqueFramebuffer & framebuffer, vk::Rect2D renderArea, vk::ClearValue clearColor = { {std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }} }, vk::SubpassContents contents = vk::SubpassContents::eInline) const;
        void endRenderPass() const;
        
        void bindPipeline(const vk::UniquePipeline & pipeline, vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) const;
        void bindDescriptorSet(const vk::UniquePipelineLayout & layout, const vk::UniqueDescriptorSet & set, vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) const;
        void bindVertexBuffer(const vk::UniqueBuffer & buffer) const;
        void bindIndexBuffer(const vk::UniqueBuffer & buffer, vk::IndexType type = vk::IndexType::eUint16) const;

        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) const;
    private:
        vk::UniqueCommandBuffer m_commandBuffer;
    };

    static_assert(std::is_move_constructible_v<CommandBuffer>);
    static_assert(std::is_move_assignable_v<CommandBuffer>);
    static_assert(!std::is_copy_constructible_v<CommandBuffer>);
    static_assert(!std::is_copy_assignable_v<CommandBuffer>);
}
