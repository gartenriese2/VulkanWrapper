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
        m_indexBufferMemory.reset(nullptr);
        m_vertexBuffer.reset(nullptr);
        m_vertexBufferMemory.reset(nullptr);
    }
}