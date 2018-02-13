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
    };

    static_assert(std::is_move_constructible_v<Model>);
    static_assert(!std::is_copy_constructible_v<Model>);
    static_assert(std::is_move_assignable_v<Model>);
    static_assert(!std::is_copy_assignable_v<Model>);
}
