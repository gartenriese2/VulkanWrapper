#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

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
        virtual ~Model();

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

        void translate(const glm::vec3 & translate);
        void scale(const glm::vec3 & scale);
        void rotate(const glm::vec3 & axis, const float radians);

        void createBuffers(const vk::Device & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue);
        void pushConstants(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniquePipelineLayout & pipelineLayout) const;
        void draw(const vk::UniqueCommandBuffer & commandBuffer) const;

        void reset();
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
