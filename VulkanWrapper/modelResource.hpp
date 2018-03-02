#pragma once

#include <set>

#include "vertex.hpp"

namespace vw::scene
{
    template <VertexDescription VD>
    class ModelResource
    {
    public:
        ModelResource(std::vector<Vertex<VD>> && vertices, std::vector<uint32_t> && indices, const vk::UniqueDevice & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue);
        ModelResource(const ModelResource &) = delete;
        ModelResource(ModelResource && other) = default;
        ModelResource & operator=(const ModelResource &) = delete;
        ModelResource & operator=(ModelResource && other) = default;

        void draw(const std::set<vk::DeviceSize> & dynamicOffsets, const vk::UniqueCommandBuffer & cmdBuffer, const vk::UniquePipelineLayout & pipelineLayout, const vk::UniqueDescriptorSet & descriptorSet) const;
    private:
        std::vector<Vertex<VD>> m_vertices;
        std::vector<uint32_t> m_indices;

        vk::UniqueDeviceMemory m_bufferMemory;
        vk::UniqueBuffer m_buffer;
        vk::DeviceSize m_offset = 0;
    };
}
