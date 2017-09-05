#include "buffer.hpp"

#include "commandbuffer.hpp"

namespace bmvk
{
    Buffer::Buffer(const vk::Device & device, const vk::DeviceSize size, const vk::BufferUsageFlags usage)
        : m_buffer{ device.createBufferUnique({ {}, size, usage }) }
    {
    }

    vk::MemoryRequirements Buffer::getMemoryRequirements(const vk::Device & device) const
    {
        return device.getBufferMemoryRequirements(*m_buffer);
    }

    void Buffer::bindToMemory(const vk::Device & device, const vk::UniqueDeviceMemory & memory, const vk::DeviceSize offset) const
    {
        device.bindBufferMemory(*m_buffer, *memory, offset);
    }

    void Buffer::copyToImage(CommandBuffer & cmdBuffer, vk::UniqueImage & image, uint32_t width, uint32_t height) const
    {
        vk::BufferImageCopy region{ 0, 0, 0, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, {}, { width, height, 1 } };
        cmdBuffer.copyBufferToImage(m_buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    }

    void Buffer::copyToBuffer(CommandBuffer & cmdBuffer, vk::UniqueBuffer & buffer, vk::DeviceSize size) const
    {
        cmdBuffer.copyBuffer(m_buffer, buffer, size);
    }
}
