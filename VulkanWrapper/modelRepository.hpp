#pragma once

#include "modelId.hpp"
#include "modelResource.hpp"
#include "modelResourceId.hpp"
#include "vertex.hpp"

#include <set>
#include <unordered_set>
#include <unordered_map>

namespace vw::scene
{
    template<VertexDescription VD>
    class ModelRepository
    {
    public:
        ModelRepository(const vk::UniqueDevice & device, const vk::PhysicalDevice & physicalDevice, const uint32_t maxInstances);
        ModelRepository(const ModelRepository &) = delete;
        ModelRepository(ModelRepository && other) = default;
        ModelRepository & operator=(const ModelRepository &) = delete;
        ModelRepository & operator=(ModelRepository && other) = default;
        ~ModelRepository();

        ModelResourceID addResource(std::vector<Vertex<VD>> && vertices, std::vector<uint32_t> && indices, const vk::UniqueDevice & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue);
        ModelID createInstance(const ModelResourceID & resourceId);
        std::vector<ModelID> createInstances(const ModelResourceID & resourceId, const uint32_t numInstances);
        void destroyInstance(const ModelID & id);
        void destroyInstances(const std::vector<ModelID> & ids);

        void setModelMatrix(const ModelID id, const glm::mat4 & modelMatrix) const;
        void translate(const ModelID id, const glm::vec3 & translate) const;
        void scale(const ModelID id, const glm::vec3 & scale) const;
        void rotate(const ModelID id, const glm::vec3 & axis, const float radians);

        vk::DescriptorBufferInfo getDescriptorBufferInfo() const;
        vk::WriteDescriptorSet getWriteDescriptorSet(const vk::UniqueDescriptorSet & descriptorSet, const uint32_t binding, const vk::DescriptorBufferInfo & info) const;

        void flushDynamicBuffer(const vk::UniqueDevice & device) const;
        void draw(const vk::UniqueCommandBuffer & cmdBuffer, const vk::UniquePipelineLayout & pipelineLayout, const vk::UniqueDescriptorSet & descriptorSet) const;
    private:
        struct DynamicUniformBufferObject
        {
            glm::mat4 * model = nullptr;
        } m_dynamicUniformBufferObject;

        std::unordered_set<ModelResourceID, ModelResourceID::KeyHash> m_resourceIds;
        std::unordered_set<ModelID, ModelID::KeyHash> m_modelIds;
        std::unordered_map<ModelResourceID, ModelResource<VD>, ModelResourceID::KeyHash> m_resourceMap;
        std::unordered_map<ModelResourceID, std::vector<ModelID>, ModelResourceID::KeyHash> m_instanceMap;
        std::unordered_map<ModelID, vk::DeviceSize, ModelID::KeyHash> m_modelToOffsetMap;
        std::unordered_map<ModelResourceID, std::set<vk::DeviceSize>, ModelResourceID::KeyHash> m_dynamicOffsetMap;
        std::set<vk::DeviceSize> m_freeMatrixSpaces;

        vk::UniqueDeviceMemory m_dynamicUniformBufferMemory;
        vk::UniqueBuffer m_dynamicUniformBuffer;
        vk::DeviceSize m_dynamicAlignment;
        vk::DeviceSize m_bufferSize;
        void * m_mappedMemory;

        vk::DeviceSize calculateDynamicAlignment(const vk::PhysicalDeviceProperties & properties, const vk::DeviceSize initialAlignment) const;
        ModelID addInstance(const ModelResourceID & resourceId);
    };
}
