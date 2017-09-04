#include "bufferFactory.hpp"

#include "physicalDevice.hpp"

namespace bmvk
{
    BufferFactory::BufferFactory()
    {
    }

    StagingBuffer BufferFactory::createStagingBuffer(const vk::Device & device, const PhysicalDevice & physicalDevice, const vk::DeviceSize size) const
    {
        Buffer buffer{ device, size, vk::BufferUsageFlagBits::eTransferSrc };

        const auto memRequirements{ buffer.getMemoryRequirements(device) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, physicalDevice.findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };
        auto memory = device.allocateMemoryUnique(allocInfo);

        buffer.bindToMemory(device, memory);

        return StagingBuffer(std::move(buffer), std::move(memory));
    }
}
