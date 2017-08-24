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

    void CommandBuffer::copyBuffer(const vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const
    {
        m_commandBuffer->copyBuffer(*srcBuffer, *dstBuffer, { { 0, 0, size } });
    }

    void CommandBuffer::copyBufferToImage(const vk::UniqueBuffer & srcBuffer, vk::UniqueImage & dstImage, vk::ImageLayout dstImageLayout, vk::ArrayProxy<const vk::BufferImageCopy> regions) const
    {
        m_commandBuffer->copyBufferToImage(*srcBuffer, *dstImage, dstImageLayout, regions);
    }

    void CommandBuffer::beginRenderPass(const vk::UniqueRenderPass & renderPass, const vk::UniqueFramebuffer & framebuffer, vk::Rect2D renderArea, vk::ClearValue clearColor, vk::SubpassContents contents) const
    {
        m_commandBuffer->beginRenderPass({ *renderPass, *framebuffer, renderArea, 1, &clearColor }, contents);
    }

    void CommandBuffer::endRenderPass() const
    {
        m_commandBuffer->endRenderPass();
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

    void CommandBuffer::bindVertexBuffer(const vk::UniqueBuffer & buffer) const
    {
        m_commandBuffer->bindVertexBuffers(0, *buffer, {0});
    }

    void CommandBuffer::bindIndexBuffer(const vk::UniqueBuffer & buffer, vk::IndexType type) const
    {
        m_commandBuffer->bindIndexBuffer(*buffer, {}, type);
    }

    void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const
    {
        m_commandBuffer->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}
