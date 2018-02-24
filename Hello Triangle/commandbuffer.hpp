#pragma once

#include <vulkan/vulkan.hpp>

#include <type_traits>

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

        void copyBuffer(const vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size, vk::DeviceSize srcOffset = 0, vk::DeviceSize dstOffset = 0) const;
        void copyBufferToImage(const vk::UniqueBuffer & srcBuffer, vk::UniqueImage & dstImage, vk::ImageLayout dstImageLayout, vk::ArrayProxy<const vk::BufferImageCopy> regions) const;

        void beginRenderPass(const vk::UniqueRenderPass & renderPass, const vk::UniqueFramebuffer & framebuffer, vk::Rect2D renderArea, vk::ArrayProxy<vk::ClearValue> clearColors, vk::SubpassContents contents = vk::SubpassContents::eInline) const;
        void endRenderPass() const;

        void setViewport(vk::Viewport viewport) const;
        void setScissor(vk::ArrayProxy<const vk::Rect2D> scissors) const;

        template<class T>
        void pushConstants(const vk::UniquePipelineLayout & layout, vk::ShaderStageFlags stageFlags, uint32_t offset, std::vector<T> values) const;

        void pipelineBarrier(vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask, vk::DependencyFlags dependencyFlags, vk::ArrayProxy<const vk::MemoryBarrier> memoryBarriers, vk::ArrayProxy<const vk::BufferMemoryBarrier> bufferMemoryBarriers, vk::ArrayProxy<const vk::ImageMemoryBarrier> imageMemoryBarriers) const;
        
        void bindPipeline(const vk::UniquePipeline & pipeline, vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) const;
        void bindDescriptorSet(const vk::UniquePipelineLayout & layout, const vk::UniqueDescriptorSet & set, vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) const;
        void bindVertexBuffer(const vk::UniqueBuffer & buffer, const vk::DeviceSize offset = 0) const;
        void bindIndexBuffer(const vk::UniqueBuffer & buffer, vk::IndexType type = vk::IndexType::eUint16, const vk::DeviceSize offset = 0) const;

        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) const;
    private:
        vk::UniqueCommandBuffer m_commandBuffer;
    };

    template<class T>
    void CommandBuffer::pushConstants(const vk::UniquePipelineLayout & layout, vk::ShaderStageFlags stageFlags, uint32_t offset, std::vector<T> values) const
    {
        m_commandBuffer->pushConstants(*layout, stageFlags, sizeof(T) * offset, sizeof(T) * static_cast<uint32_t>(values.size()), reinterpret_cast<void *>(values.data()));
    }

    static_assert(std::is_move_constructible_v<CommandBuffer>);
    static_assert(std::is_move_assignable_v<CommandBuffer>);
    static_assert(!std::is_copy_constructible_v<CommandBuffer>);
    static_assert(!std::is_copy_assignable_v<CommandBuffer>);
}
