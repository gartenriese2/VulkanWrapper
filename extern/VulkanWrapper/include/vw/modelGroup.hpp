#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "vertex.hpp"
#include "util.hpp"
#include "modelId.hpp"

namespace vw::scene
{
    template<VertexDescription VD>
    class ModelGroup
    {
    public:
        ModelGroup(const vk::PhysicalDeviceProperties & prop, const uint32_t maxNumInstances)
          : m_maxNumInstances{ maxNumInstances },
            m_numInstances{ 0 },
            m_dynamicAlignment{ sizeof(glm::mat4) }
        {
            const auto minUboAlignment = prop.limits.minUniformBufferOffsetAlignment;
            if (minUboAlignment > 0)
            {
                m_dynamicAlignment = (m_dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
            }

            const auto bufferSize{ m_maxNumInstances * m_dynamicAlignment };

            m_dynamicUniformBufferObject.model = static_cast<glm::mat4 *>(_aligned_malloc(bufferSize, m_dynamicAlignment));
            if (m_dynamicUniformBufferObject.model == nullptr)
            {
                throw std::runtime_error("model matrix pointer must not be null");
            }
        }
        ModelGroup(ModelGroup && other) = default;
        ModelGroup & operator=(ModelGroup && other) = default;
        ~ModelGroup()
        {
            _aligned_free(m_dynamicUniformBufferObject.model);
        }

        void setVertices(const std::vector<Vertex<VD>> & vertices)
        {
            m_vertices = vertices;
        }

        void setIndices(const std::vector<uint32_t> & indices)
        {
            m_indices = indices;
        }

        auto getDescriptorBufferInfo()
        {
            return vk::DescriptorBufferInfo{ *m_dynamicUniformBuffer, 0, sizeof(DynamicUniformBufferObject) };
        }

        void createBuffers(const vk::UniqueDevice & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue)
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

            // Create dynamic buffer
            const auto dynamicBufferSize{ m_maxNumInstances * m_dynamicAlignment };
            util::createBuffer(device, physicalDevice, dynamicBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible, m_dynamicUniformBuffer, m_dynamicUniformBufferMemory);
        }

        void draw(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniquePipelineLayout & pipelineLayout, const vk::UniqueDescriptorSet & desciptorSet) const
        {
            vk::DeviceSize offsets = 0;
            commandBuffer->bindVertexBuffers(0, *m_buffer, offsets);
            commandBuffer->bindIndexBuffer(*m_buffer, m_offset, vk::IndexType::eUint32);

            for (uint32_t i = 0; i < m_numInstances; ++i)
            {
                auto dynamicOffset = i * static_cast<uint32_t>(m_dynamicAlignment);
                commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *desciptorSet, dynamicOffset);
                commandBuffer->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
            }
        }

        void flush(const vk::UniqueDevice & device)
        {
            const auto bufSize{ m_maxNumInstances * m_dynamicAlignment };
            auto data{ device->mapMemory(*m_dynamicUniformBufferMemory, 0, bufSize, {}) };
            memcpy(data, m_dynamicUniformBufferObject.model, bufSize);
            vk::MappedMemoryRange mmr{ *m_dynamicUniformBufferMemory, 0, bufSize };
            device->flushMappedMemoryRanges(mmr);
            device->unmapMemory(*m_dynamicUniformBufferMemory);
        }

        void setModelMatrix(const ModelID id, const glm::mat4 & modelMatrix)
        {
            if (!idExists(id))
            {
                throw std::invalid_argument("modelgroup does not contain this ID");
            }

            const auto idx{ m_idToIdxMap[id] };
            glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + idx * m_dynamicAlignment));
            *modelMat = modelMatrix;
        }

        void translate(const ModelID id, const glm::vec3 & translate)
        {
            if (!idExists(id))
            {
                throw std::invalid_argument("modelgroup does not contain this ID");
            }

            const auto idx{ m_idToIdxMap[id] };
            glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + (idx * m_dynamicAlignment)));
            *modelMat = glm::translate(*modelMat, translate);
        }

        void scale(const uint32_t id, const glm::vec3 & scale)
        {
            if (!idExists(id))
            {
                throw std::invalid_argument("modelgroup does not contain this ID");
            }

            const auto idx{ m_idToIdxMap[id] };
            glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + (idx * m_dynamicAlignment)));
            *modelMat = glm::scale(*modelMat, scale);
        }

        void rotate(const uint32_t id, const glm::vec3 & axis, const float radians)
        {
            if (!idExists(id))
            {
                throw std::invalid_argument("modelgroup does not contain this ID");
            }

            const auto idx{ m_idToIdxMap[id] };
            glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + (idx * m_dynamicAlignment)));
            *modelMat = glm::rotate(*modelMat, radians, axis);
        }

        auto getNumInstances() const noexcept { return m_numInstances; }

        ModelID addInstance()
        {
            if (m_numInstances + 1 > m_maxNumInstances)
            {
                throw std::invalid_argument("too many instances");
            }

            ModelID id;
            m_idToIdxMap[id] = m_numInstances++;
            return id;
        }

        std::vector<ModelID> addInstances(const uint32_t numAdditionalInstances)
        {
            if (m_numInstances + numAdditionalInstances > m_maxNumInstances)
            {
                throw std::invalid_argument("too many instances");
            }

            std::vector<ModelID> ret;
            for (uint32_t i = 0; i < numAdditionalInstances; ++i)
            {
                ret.emplace_back(addInstance());
            }

            return ret;
        }

        void clear() { m_numInstances = 0; m_idToIdxMap.clear(); }
    private:
        bool idExists(const ModelID id) const { return m_idToIdxMap.find(id) != m_idToIdxMap.end(); }
        
        std::unordered_map<ModelID, uint32_t, ModelID::KeyHash> m_idToIdxMap;

        struct DynamicUniformBufferObject
        {
            glm::mat4 * model = nullptr;
        } m_dynamicUniformBufferObject;

        uint32_t m_maxNumInstances;
        uint32_t m_numInstances;
        size_t m_dynamicAlignment = 0;

        std::vector<Vertex<VD>> m_vertices;
        std::vector<uint32_t> m_indices;

        vk::UniqueDeviceMemory m_dynamicUniformBufferMemory;
        vk::UniqueBuffer m_dynamicUniformBuffer;

        vk::UniqueDeviceMemory m_bufferMemory;
        vk::UniqueBuffer m_buffer;
        vk::DeviceSize m_offset = 0;
    };

    static_assert(std::is_move_constructible_v<ModelGroup<VertexDescription::PositionNormalColorTexture>>);
    static_assert(!std::is_copy_constructible_v<ModelGroup<VertexDescription::PositionNormalColorTexture>>);
    static_assert(std::is_move_assignable_v<ModelGroup<VertexDescription::PositionNormalColorTexture>>);
    static_assert(!std::is_copy_assignable_v<ModelGroup<VertexDescription::PositionNormalColorTexture>>);

    static_assert(std::is_move_constructible_v<ModelGroup<VertexDescription::PositionNormalColor>>);
    static_assert(!std::is_copy_constructible_v<ModelGroup<VertexDescription::PositionNormalColor>>);
    static_assert(std::is_move_assignable_v<ModelGroup<VertexDescription::PositionNormalColor>>);
    static_assert(!std::is_copy_assignable_v<ModelGroup<VertexDescription::PositionNormalColor>>);
}
