#include "buffer.hpp"

#include "commandbuffer.hpp"

namespace bmvk
{
    Buffer::Buffer(const vk::UniqueDevice & device, const vk::DeviceSize size, const vk::BufferUsageFlags usage, const vk::SharingMode sharingMode)
        : m_buffer{ device->createBufferUnique({ {}, size, usage, sharingMode }) }
    {
    }

    vk::MemoryRequirements Buffer::getMemoryRequirements(const vk::UniqueDevice & device) const
    {
        return device->getBufferMemoryRequirements(*m_buffer);
    }

    void Buffer::bindToMemory(const vk::UniqueDevice & device, const vk::UniqueDeviceMemory & memory, const vk::DeviceSize offset) const
    {
        device->bindBufferMemory(*m_buffer, *memory, offset);
    }

    void Buffer::copyToImage(CommandBuffer & cmdBuffer, vk::UniqueImage & image, uint32_t width, uint32_t height) const
    {
        vk::BufferImageCopy region{ 0, 0, 0, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, {}, { width, height, 1 } };
        cmdBuffer.copyBufferToImage(m_buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    }

    void Buffer::copyToBuffer(CommandBuffer & cmdBuffer, vk::UniqueBuffer & buffer, vk::DeviceSize size, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset) const
    {
        cmdBuffer.copyBuffer(m_buffer, buffer, size, srcOffset, dstOffset);
    }
}
