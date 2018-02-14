#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>

#include <type_traits>

#include "vertex.hpp"

namespace vw::util
{
    class Model
    {
    public:
        Model() {}
        Model(Model && other) = default;
        Model & operator=(Model && other) = default;

        const auto & getVertices() const noexcept { return m_vertices; }
        auto & getVertices() noexcept { return m_vertices; }
        const auto & getIndices() const noexcept { return m_indices; }
        auto & getIndices() noexcept { return m_indices; }
        const auto & getVertexBuffer() const noexcept { return m_vertexBuffer; }
        auto & getVertexBuffer() noexcept { return m_vertexBuffer; }
        const auto & getVertexBufferMemory() const noexcept { return m_vertexBufferMemory; }
        auto & getVertexBufferMemory() noexcept { return m_vertexBufferMemory; }
        const auto & getIndexBuffer() const noexcept { return m_indexBuffer; }
        auto & getIndexBuffer() noexcept { return m_indexBuffer; }
        const auto & getIndexBufferMemory() const noexcept { return m_indexBufferMemory; }
        auto & getIndexBufferMemory() noexcept { return m_indexBufferMemory; }

        void translate(const glm::vec3 & translate)
        {
            m_modelMatrix = glm::translate(m_modelMatrix, translate);
        }

        void scale(const glm::vec3 & scale)
        {
            m_modelMatrix = glm::scale(m_modelMatrix, scale);
        }

        void rotate(const glm::vec3 & axis, const float radians)
        {
            m_modelMatrix = glm::rotate(m_modelMatrix, radians, axis);
        }

        void createBuffers(const vk::Device & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue)
        {
            {
                vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

                vk::UniqueBuffer stagingBuffer;
                vk::UniqueDeviceMemory stagingBufferMemory;
                createBuffer(device, physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

                auto * data{ device.mapMemory(*stagingBufferMemory, 0, bufferSize,{}) };
                memcpy(data, m_vertices.data(), static_cast<size_t>(bufferSize));
                device.unmapMemory(*stagingBufferMemory);

                createBuffer(device, physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_vertexBuffer, m_vertexBufferMemory);

                copyBuffer(device, commandPool, queue, stagingBuffer, m_vertexBuffer, bufferSize);

                stagingBuffer.reset(nullptr);
                stagingBufferMemory.reset(nullptr);
            }
            
            {
                vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

                vk::UniqueBuffer stagingBuffer;
                vk::UniqueDeviceMemory stagingBufferMemory;
                createBuffer(device, physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

                auto * data{ device.mapMemory(*stagingBufferMemory, 0, bufferSize,{}) };
                memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
                device.unmapMemory(*stagingBufferMemory);

                createBuffer(device, physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_indexBuffer, m_indexBufferMemory);

                copyBuffer(device, commandPool, queue, stagingBuffer, m_indexBuffer, bufferSize);

                stagingBuffer.reset(nullptr);
                stagingBufferMemory.reset(nullptr);
            }
        }

        void pushConstants(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniquePipelineLayout & pipelineLayout) const
        {
            std::array<glm::mat4, 1> pushConstants = { m_modelMatrix };
            commandBuffer->pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof pushConstants, pushConstants.data());
        }

        void draw(const vk::UniqueCommandBuffer & commandBuffer) const
        {
            vk::DeviceSize offsets = 0;
            commandBuffer->bindVertexBuffers(0, *m_vertexBuffer, offsets);
            commandBuffer->bindIndexBuffer(*m_indexBuffer, 0, vk::IndexType::eUint32);
            commandBuffer->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        }

        void reset()
        {
            m_indexBuffer.reset(nullptr);
            m_indexBufferMemory.reset(nullptr);
            m_vertexBuffer.reset(nullptr);
            m_vertexBufferMemory.reset(nullptr);
        }
    private:
        glm::mat4 m_modelMatrix;

        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        vk::UniqueBuffer m_vertexBuffer;
        vk::UniqueDeviceMemory m_vertexBufferMemory;
        vk::UniqueBuffer m_indexBuffer;
        vk::UniqueDeviceMemory m_indexBufferMemory;

        static uint32_t findMemoryType(const vk::PhysicalDevice & physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
        {
            const auto memProperties{ physicalDevice.getMemoryProperties() };
            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
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
    };

    static_assert(std::is_move_constructible_v<Model>);
    static_assert(!std::is_copy_constructible_v<Model>);
    static_assert(std::is_move_assignable_v<Model>);
    static_assert(!std::is_copy_assignable_v<Model>);
}
