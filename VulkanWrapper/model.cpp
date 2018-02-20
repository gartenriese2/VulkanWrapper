#include "model.hpp"

#include "util.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace vw::scene
{
    void Model::translate(const glm::vec3 & translate)
    {
        m_modelMatrix = glm::translate(m_modelMatrix, translate);
    }

    void Model::scale(const glm::vec3 & scale)
    {
        m_modelMatrix = glm::scale(m_modelMatrix, scale);
    }

    void Model::rotate(const glm::vec3 & axis, const float radians)
    {
        m_modelMatrix = glm::rotate(m_modelMatrix, radians, axis);
    }

    void Model::createBuffers(const vk::Device & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue)
    {
        const auto vertexBufferSize{ sizeof(m_vertices[0]) * m_vertices.size() };
        const auto indexBufferSize{ sizeof(m_indices[0]) * m_indices.size() };

        // Create & fill staging buffers & memories
        vk::UniqueBuffer vertexStagingBuffer;
        vk::UniqueDeviceMemory vertexStagingBufferMemory;
        util::createBuffer(device, physicalDevice, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexStagingBuffer, vertexStagingBufferMemory);

        auto * vertexData{ device.mapMemory(*vertexStagingBufferMemory, 0, vertexBufferSize, {}) };
        memcpy(vertexData, m_vertices.data(), static_cast<size_t>(vertexBufferSize));
        device.unmapMemory(*vertexStagingBufferMemory);

        vk::UniqueBuffer indexStagingBuffer;
        vk::UniqueDeviceMemory indexStagingBufferMemory;
        util::createBuffer(device, physicalDevice, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexStagingBuffer, indexStagingBufferMemory);

        auto * indexData{ device.mapMemory(*indexStagingBufferMemory, 0, indexBufferSize, {}) };
        memcpy(indexData, m_indices.data(), static_cast<size_t>(indexBufferSize));
        device.unmapMemory(*indexStagingBufferMemory);

        // Get size & offset
        const auto vb = device.createBufferUnique({ {}, vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer });
        const auto ib = device.createBufferUnique({ {}, indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer });
        const auto vMemReq{ device.getBufferMemoryRequirements(*vb) };
        const auto iMemReq{ device.getBufferMemoryRequirements(*vb) };
        const auto bufSize{ vMemReq.size + iMemReq.size };
        m_offset = vMemReq.size;

        // Create buffer & memory
        util::createBuffer(device, physicalDevice, bufSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_buffer, m_bufferMemory);

        // Copy staging buffers into buffer
        auto vec = device.allocateCommandBuffersUnique({ *commandPool, vk::CommandBufferLevel::ePrimary, 1 });
        if (vec.size() != 1)
        {
            throw std::runtime_error("allocating single command buffer failed, created " + std::to_string(vec.size()) + " command buffers instead.");
        }
        auto cmdBuffer{ std::move(vec[0]) };
        cmdBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        cmdBuffer->copyBuffer(*vertexStagingBuffer, *m_buffer, { { 0, 0, vertexBufferSize } });
        cmdBuffer->copyBuffer(*indexStagingBuffer, *m_buffer, { { 0, m_offset, indexBufferSize } });
        cmdBuffer->end();
        vk::CommandBuffer commandBuffers[] = { *cmdBuffer };
        const vk::SubmitInfo info(0, nullptr, nullptr, 1, commandBuffers, 0, nullptr);
        queue.submit(info, nullptr);
        queue.waitIdle();

        // Destroy staging buffers before staging memories
        vertexStagingBuffer.reset(nullptr);
        indexStagingBuffer.reset(nullptr);
    }

    void Model::pushConstants(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniquePipelineLayout & pipelineLayout) const
    {
        std::array<glm::mat4, 1> pushConstants = { m_modelMatrix };
        commandBuffer->pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstants), pushConstants.data());
    }

    void Model::draw(const vk::UniqueCommandBuffer & commandBuffer) const
    {
        vk::DeviceSize offsets = 0;
        commandBuffer->bindVertexBuffers(0, *m_buffer, offsets);
        commandBuffer->bindIndexBuffer(*m_buffer, m_offset, vk::IndexType::eUint32);
        commandBuffer->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }

    void Model::drawInstanced(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniquePipelineLayout & pipelineLayout, const vk::UniqueDescriptorSet & desciptorSet, const uint32_t num, const size_t dynamicAlignment) const
    {
        vk::DeviceSize offsets = 0;
        commandBuffer->bindVertexBuffers(0, *m_buffer, offsets);
        commandBuffer->bindIndexBuffer(*m_buffer, m_offset, vk::IndexType::eUint32);

        for (uint32_t i = 0; i < num; ++i)
        {
            auto dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);
            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *desciptorSet, dynamicOffset);
            commandBuffer->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        }
    }

    void Model::reset()
    {
        m_buffer.reset(nullptr);
        m_bufferMemory.reset(nullptr);
    }
}