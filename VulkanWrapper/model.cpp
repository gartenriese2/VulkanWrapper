#include "model.hpp"

#include "util.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace vw::util
{
    Model::~Model()
    {
        reset();
    }

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

        vk::UniqueBuffer vertexStagingBuffer;
        vk::UniqueDeviceMemory vertexStagingBufferMemory;
        createBuffer(device, physicalDevice, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexStagingBuffer, vertexStagingBufferMemory);

        auto * vertexData{ device.mapMemory(*vertexStagingBufferMemory, 0, vertexBufferSize, {}) };
        memcpy(vertexData, m_vertices.data(), static_cast<size_t>(vertexBufferSize));
        device.unmapMemory(*vertexStagingBufferMemory);

        vk::UniqueBuffer indexStagingBuffer;
        vk::UniqueDeviceMemory indexStagingBufferMemory;
        createBuffer(device, physicalDevice, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexStagingBuffer, indexStagingBufferMemory);

        auto * indexData{ device.mapMemory(*indexStagingBufferMemory, 0, indexBufferSize, {}) };
        memcpy(indexData, m_indices.data(), static_cast<size_t>(indexBufferSize));
        device.unmapMemory(*indexStagingBufferMemory);

        const auto vertexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer };
        const auto indexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer };
        const auto bufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eDeviceLocal };

        vk::BufferCreateInfo vertexBufferInfo{ {}, vertexBufferSize, vertexBufferUsageFlags };
        m_vertexBuffer = device.createBufferUnique(vertexBufferInfo);

        vk::BufferCreateInfo indexBufferInfo{ {}, indexBufferSize, indexBufferUsageFlags };
        m_indexBuffer = device.createBufferUnique(indexBufferInfo);

        const auto vertexMemRequirements{ device.getBufferMemoryRequirements(*m_vertexBuffer) };
        const auto indexMemRequirements{ device.getBufferMemoryRequirements(*m_indexBuffer) };
        const auto vertexMemoryType{ findMemoryType(physicalDevice, vertexMemRequirements.memoryTypeBits, bufferMemoryPropertyFlags) };
        const auto indexMemoryType{ findMemoryType(physicalDevice, indexMemRequirements.memoryTypeBits, bufferMemoryPropertyFlags) };

        if (vertexMemoryType == indexMemoryType)
        {
            vk::MemoryAllocateInfo allocInfo{ vertexMemRequirements.size + indexMemRequirements.size, vertexMemoryType };
            m_combinedBufferMemory = device.allocateMemoryUnique(allocInfo);
        }
        else
        {
            throw std::runtime_error("memory can't be combined!");
        }

        device.bindBufferMemory(*m_vertexBuffer, *m_combinedBufferMemory, 0);
        device.bindBufferMemory(*m_indexBuffer, *m_combinedBufferMemory, vertexMemRequirements.size);

        auto vec = device.allocateCommandBuffersUnique({ *commandPool, vk::CommandBufferLevel::ePrimary, 1 });
        if (vec.size() != 1)
        {
            throw std::runtime_error("allocating single command buffer failed, created " + std::to_string(vec.size()) + " command buffers instead.");
        }
        auto cmdBuffer{ std::move(vec[0]) };
        cmdBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        cmdBuffer->copyBuffer(*vertexStagingBuffer, *m_vertexBuffer, { { 0, 0, vertexBufferSize } });
        cmdBuffer->copyBuffer(*indexStagingBuffer, *m_indexBuffer, { { 0, 0, indexBufferSize } });
        cmdBuffer->end();
        vk::CommandBuffer commandBuffers[] = { *cmdBuffer };
        const vk::SubmitInfo info(0, nullptr, nullptr, 1, commandBuffers, 0, nullptr);
        queue.submit(info, nullptr);
        queue.waitIdle();

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
        commandBuffer->bindVertexBuffers(0, *m_vertexBuffer, offsets);
        commandBuffer->bindIndexBuffer(*m_indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffer->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }

    void Model::reset()
    {
        m_indexBuffer.reset(nullptr);
        m_vertexBuffer.reset(nullptr);
        m_combinedBufferMemory.reset(nullptr);
    }
}