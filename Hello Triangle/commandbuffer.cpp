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

    void CommandBuffer::copyBuffer(vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const
    {
        m_commandBuffer->copyBuffer(*srcBuffer, *dstBuffer, { { 0, 0, size } });
    }
}
