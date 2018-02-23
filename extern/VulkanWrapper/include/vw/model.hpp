#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#include <type_traits>

#include "vertex.hpp"

namespace vw::scene
{
    template<VertexDescription VD>
    class Model
    {
    public:
        Model() {}
        Model(Model && other) = default;
        Model & operator=(Model && other) = default;

        const auto & getModelMatrix() const noexcept { return m_modelMatrix; }

        const auto & getVertices() const noexcept { return m_vertices; }
        auto & getVertices() noexcept { return m_vertices; }
        const auto & getIndices() const noexcept { return m_indices; }
        auto & getIndices() noexcept { return m_indices; }

        void translate(const glm::vec3 & translate);
        void scale(const glm::vec3 & scale);
        void rotate(const glm::vec3 & axis, const float radians);

        void createBuffers(const vk::UniqueDevice & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue);
        void pushConstants(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniquePipelineLayout & pipelineLayout) const;
        void draw(const vk::UniqueCommandBuffer & commandBuffer) const;
        void drawInstanced(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniquePipelineLayout & pipelineLayout, const vk::UniqueDescriptorSet & desciptorSet, const uint32_t num, const size_t dynamicAlignment) const;

        void reset();
    private:
        glm::mat4 m_modelMatrix;

        std::vector<Vertex<VD>> m_vertices;
        std::vector<uint32_t> m_indices;
        vk::UniqueDeviceMemory m_bufferMemory;
        vk::UniqueBuffer m_buffer;
        vk::DeviceSize m_offset = 0;
    };

    static_assert(std::is_move_constructible_v<Model<VertexDescription::PositionNormalColorTexture>>);
    static_assert(!std::is_copy_constructible_v<Model<VertexDescription::PositionNormalColorTexture>>);
    static_assert(std::is_move_assignable_v<Model<VertexDescription::PositionNormalColorTexture>>);
    static_assert(!std::is_copy_assignable_v<Model<VertexDescription::PositionNormalColorTexture>>);

    static_assert(std::is_move_constructible_v<Model<VertexDescription::PositionNormalColor>>);
    static_assert(!std::is_copy_constructible_v<Model<VertexDescription::PositionNormalColor>>);
    static_assert(std::is_move_assignable_v<Model<VertexDescription::PositionNormalColor>>);
    static_assert(!std::is_copy_assignable_v<Model<VertexDescription::PositionNormalColor>>);
}
