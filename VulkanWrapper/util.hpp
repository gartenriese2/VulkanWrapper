#pragma once

#include <vulkan/vulkan.hpp>

namespace vw::util
{
    static uint32_t findMemoryType(const vk::PhysicalDevice & physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        const auto memProperties{ physicalDevice.getMemoryProperties() };
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    static void createBuffer(const vk::Device & device, const vk::PhysicalDevice & physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory)
    {
        vk::BufferCreateInfo bufferInfo{ {}, size, usage };
        buffer = device.createBufferUnique(bufferInfo);

        const auto memRequirements{ device.getBufferMemoryRequirements(*buffer) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) };
        bufferMemory = device.allocateMemoryUnique(allocInfo);

        device.bindBufferMemory(*buffer, *bufferMemory, 0);
    }

    static void copyBuffer(const vk::Device & device, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue, vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size)
    {
        auto vec = device.allocateCommandBuffersUnique({ *commandPool, vk::CommandBufferLevel::ePrimary, 1 });
        assert(vec.size() == 1);

        auto cmdBuffer{ std::move(vec[0]) };
        cmdBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        cmdBuffer->copyBuffer(*srcBuffer, *dstBuffer, { { 0, 0, size } });
        cmdBuffer->end();
        vk::CommandBuffer commandBuffers[] = { *cmdBuffer };
        const vk::SubmitInfo info(0, nullptr, nullptr, 1, commandBuffers, 0, nullptr);
        queue.submit(info, nullptr);
        queue.waitIdle();
    }

    static void copyBuffer(const vk::UniqueCommandBuffer & commandBuffer, const vk::Queue & queue, vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size)
    {
        commandBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        commandBuffer->copyBuffer(*srcBuffer, *dstBuffer, { { 0, 0, size } });
        commandBuffer->end();
        vk::CommandBuffer commandBuffers[] = { *commandBuffer };
        const vk::SubmitInfo info{ 0, nullptr, nullptr, 1, commandBuffers };
        queue.submit(info, nullptr);
        queue.waitIdle();
    }
}