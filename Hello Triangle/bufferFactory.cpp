#include "bufferFactory.hpp"

#include "physicalDevice.hpp"

namespace bmvk
{
    BufferFactory::BufferFactory(Device & device, const vk::PhysicalDevice & physicalDevice)
      : m_device{device},
        m_memoryProperties{ physicalDevice.getMemoryProperties() }
    {
    }

    StagingBuffer BufferFactory::createStagingBuffer(const vk::DeviceSize size) const
    {
        Buffer buffer{ static_cast<vk::Device>(m_device), size, vk::BufferUsageFlagBits::eTransferSrc };

        const auto memRequirements{ buffer.getMemoryRequirements(static_cast<vk::Device>(m_device)) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };
        auto memory = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);

        buffer.bindToMemory(static_cast<vk::Device>(m_device), memory);

        return StagingBuffer(m_device, std::move(buffer), std::move(memory));
    }

    uint32_t BufferFactory::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; ++i)
        {
            if (typeFilter & 1 << i && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }
}
