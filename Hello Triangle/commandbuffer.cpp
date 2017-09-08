#include "commandbuffer.hpp"

namespace bmvk
{
    void CommandBuffer::begin(vk::CommandBufferUsageFlags flags) const
    {
        m_commandBuffer->begin(flags);
    }

    void CommandBuffer::end() const
    {
        m_commandBuffer->end();
    }

    void CommandBuffer::copyBuffer(const vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset) const
    {
        m_commandBuffer->copyBuffer(*srcBuffer, *dstBuffer, { { srcOffset, dstOffset, size } });
    }

    void CommandBuffer::copyBufferToImage(const vk::UniqueBuffer & srcBuffer, vk::UniqueImage & dstImage, vk::ImageLayout dstImageLayout, vk::ArrayProxy<const vk::BufferImageCopy> regions) const
    {
        m_commandBuffer->copyBufferToImage(*srcBuffer, *dstImage, dstImageLayout, regions);
    }

    void CommandBuffer::beginRenderPass(const vk::UniqueRenderPass & renderPass, const vk::UniqueFramebuffer & framebuffer, vk::Rect2D renderArea, vk::ArrayProxy<vk::ClearValue> clearColors, vk::SubpassContents contents) const
    {
        m_commandBuffer->beginRenderPass({ *renderPass, *framebuffer, renderArea, clearColors.size(), clearColors.data() }, contents);
    }

    void CommandBuffer::endRenderPass() const
    {
        m_commandBuffer->endRenderPass();
    }

    void CommandBuffer::setViewport(vk::Viewport viewport) const
    {
        m_commandBuffer->setViewport(0, viewport);
    }

    void CommandBuffer::setScissor(vk::ArrayProxy<const vk::Rect2D> scissors) const
    {
        m_commandBuffer->setScissor(0, scissors);
    }

    void CommandBuffer::pipelineBarrier(vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask, vk::DependencyFlags dependencyFlags, vk::ArrayProxy<const vk::MemoryBarrier> memoryBarriers, vk::ArrayProxy<const vk::BufferMemoryBarrier> bufferMemoryBarriers, vk::ArrayProxy<const vk::ImageMemoryBarrier> imageMemoryBarriers) const
    {
        m_commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers, bufferMemoryBarriers, imageMemoryBarriers);
    }

    void CommandBuffer::bindPipeline(const vk::UniquePipeline & pipeline, vk::PipelineBindPoint bindPoint) const
    {
        m_commandBuffer->bindPipeline(bindPoint, *pipeline);
    }

    void CommandBuffer::bindDescriptorSet(const vk::UniquePipelineLayout & layout, const vk::UniqueDescriptorSet & set, vk::PipelineBindPoint bindPoint) const
    {
        m_commandBuffer->bindDescriptorSets(bindPoint, *layout, 0, *set, nullptr);
    }

    void CommandBuffer::bindVertexBuffer(const vk::UniqueBuffer & buffer, const vk::DeviceSize offset) const
    {
        m_commandBuffer->bindVertexBuffers(0, *buffer, {offset});
    }

    void CommandBuffer::bindIndexBuffer(const vk::UniqueBuffer & buffer, vk::IndexType type, const vk::DeviceSize offset) const
    {
        m_commandBuffer->bindIndexBuffer(*buffer, offset, type);
    }

    void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const
    {
        m_commandBuffer->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}
