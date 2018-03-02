#include "modelResource.hpp"

#include "util.hpp"

namespace vw::scene
{
    template<VertexDescription VD>
    ModelResource<VD>::ModelResource(std::vector<Vertex<VD>> && vertices, std::vector<uint32_t> && indices, const vk::UniqueDevice & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue)
        : m_vertices{ std::move(vertices) },
          m_indices{ std::move(indices) }
    {
        const auto vertexBufferSize{ sizeof(m_vertices[0]) * m_vertices.size() };
        const auto indexBufferSize{ sizeof(m_indices[0]) * m_indices.size() };

        // Create & fill staging buffers & memories
        vk::UniqueBuffer vertexStagingBuffer;
        vk::UniqueDeviceMemory vertexStagingBufferMemory;
        util::createBuffer(device, physicalDevice, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexStagingBuffer, vertexStagingBufferMemory);

        auto * vertexData{ device->mapMemory(*vertexStagingBufferMemory, 0, vertexBufferSize,{}) };
        memcpy(vertexData, m_vertices.data(), static_cast<size_t>(vertexBufferSize));
        device->unmapMemory(*vertexStagingBufferMemory);

        vk::UniqueBuffer indexStagingBuffer;
        vk::UniqueDeviceMemory indexStagingBufferMemory;
        util::createBuffer(device, physicalDevice, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexStagingBuffer, indexStagingBufferMemory);

        auto * indexData{ device->mapMemory(*indexStagingBufferMemory, 0, indexBufferSize,{}) };
        memcpy(indexData, m_indices.data(), static_cast<size_t>(indexBufferSize));
        device->unmapMemory(*indexStagingBufferMemory);

        // Get size & offset
        const auto vb = device->createBufferUnique({ {}, vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer });
        const auto ib = device->createBufferUnique({ {}, indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer });
        const auto vMemReq{ device->getBufferMemoryRequirements(*vb) };
        const auto iMemReq{ device->getBufferMemoryRequirements(*vb) };
        const auto bufSize{ vMemReq.size + iMemReq.size };
        m_offset = vMemReq.size;

        // Create buffer & memory
        util::createBuffer(device, physicalDevice, bufSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_buffer, m_bufferMemory);

        // Copy staging buffers into buffer
        auto vec = device->allocateCommandBuffersUnique({ *commandPool, vk::CommandBufferLevel::ePrimary, 1 });
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

    template<VertexDescription VD>
    void ModelResource<VD>::draw(const std::set<vk::DeviceSize> & dynamicOffsets, const vk::UniqueCommandBuffer & cmdBuffer, const vk::UniquePipelineLayout & pipelineLayout, const vk::UniqueDescriptorSet & descriptorSet) const
    {
        vk::DeviceSize offsets = 0;
        cmdBuffer->bindVertexBuffers(0, *m_buffer, offsets);
        cmdBuffer->bindIndexBuffer(*m_buffer, m_offset, vk::IndexType::eUint32);

        for (const auto dynamicOffset : dynamicOffsets)
        {
            cmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descriptorSet, static_cast<uint32_t>(dynamicOffset));
            cmdBuffer->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        }
    }

    template class ModelResource<VertexDescription::PositionNormalColor>;
    template class ModelResource<VertexDescription::PositionNormalColorTexture>;
}
