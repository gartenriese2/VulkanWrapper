#include "modelRepository.hpp"

#include <glm/gtc/matrix_transform.inl>

#include "util.hpp"

namespace vw::scene
{
    template<VertexDescription VD>
    ModelRepository<VD>::ModelRepository(const vk::UniqueDevice & device, const vk::PhysicalDevice & physicalDevice, const uint32_t maxInstances)
        : m_dynamicAlignment{ calculateDynamicAlignment(physicalDevice.getProperties(), sizeof(glm::mat4)) },
          m_bufferSize{ maxInstances * m_dynamicAlignment }
    {
        m_dynamicUniformBufferObject.model = static_cast<glm::mat4 *>(_aligned_malloc(m_bufferSize, m_dynamicAlignment));
        if (m_dynamicUniformBufferObject.model == nullptr)
        {
            throw std::runtime_error("model matrix pointer must not be null");
        }

        for (uint32_t i = 0; i < maxInstances; ++i)
        {
            m_freeMatrixSpaces.emplace(i);
        }

        // Create dynamic buffer
        util::createBuffer(device, physicalDevice, m_bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible, m_dynamicUniformBuffer, m_dynamicUniformBufferMemory);

        // Map dynamic buffer
        m_mappedMemory = device->mapMemory(*m_dynamicUniformBufferMemory, 0, m_bufferSize, {});
    }

    template<VertexDescription VD>
    ModelRepository<VD>::~ModelRepository()
    {
        _aligned_free(m_dynamicUniformBufferObject.model);
    }

    template<VertexDescription VD>
    ModelResourceID ModelRepository<VD>::addResource(std::vector<Vertex<VD>> && vertices, std::vector<uint32_t> && indices, const vk::UniqueDevice & device, const vk::PhysicalDevice & physicalDevice, const vk::UniqueCommandPool & commandPool, const vk::Queue & queue)
    {
        ModelResourceID id;
        m_resourceIds.emplace(id);
        m_resourceMap.emplace(id, ModelResource<VD>{ std::move(vertices), std::move(indices), device, physicalDevice, commandPool, queue });
        m_dynamicOffsetMap.emplace(id, std::set<vk::DeviceSize>{});
        m_instanceMap.emplace(id, std::vector<ModelID>{});
        return id;
    }

    template<VertexDescription VD>
    ModelID ModelRepository<VD>::createInstance(const ModelResourceID & resourceId)
    {
        const auto it{ m_resourceIds.find(resourceId) };
        if (it == m_resourceIds.end())
        {
            throw std::invalid_argument("Model resource with this ID is not in repository");
        }

        return addInstance(resourceId);
    }

    template<VertexDescription VD>
    std::vector<ModelID> ModelRepository<VD>::createInstances(const ModelResourceID & resourceId, const uint32_t numInstances)
    {
        const auto it{ m_resourceIds.find(resourceId) };
        if (it == m_resourceIds.end())
        {
            throw std::invalid_argument("Model resource with this ID is not in repository");
        }

        std::vector<ModelID> vec;
        for (uint32_t i = 0; i < numInstances; ++i)
        {
            vec.emplace_back(addInstance(resourceId));
        }

        return vec;
    }

    template<VertexDescription VD>
    void ModelRepository<VD>::destroyInstance(const ModelID & id)
    {
        if (m_modelIds.find(id) == m_modelIds.end())
        {
            throw std::invalid_argument("modelrepository does not contain this modelID");
        }

        m_modelIds.erase(id);
        const auto offset{ m_modelToOffsetMap[id] };
        for (auto & pair : m_instanceMap)
        {
            const auto & resId{ pair.first };
            auto & vec{ pair.second };
            const auto it{ std::find(vec.begin(), vec.end(), id) };
            if (it == vec.end())
            {
                continue;
            }

            vec.erase(it);
            m_dynamicOffsetMap.find(resId)->second.erase(offset);
        }

        m_modelToOffsetMap.erase(id);
        const auto idx{ offset / m_dynamicAlignment };
        m_freeMatrixSpaces.emplace(idx);
    }

    template<VertexDescription VD>
    void ModelRepository<VD>::destroyInstances(const std::vector<ModelID> & ids)
    {
        for (const auto id : ids)
        {
            destroyInstance(id);
        }
    }

    template<VertexDescription VD>
    void ModelRepository<VD>::setModelMatrix(const ModelID id, const glm::mat4 & modelMatrix) const
    {
        if (m_modelIds.find(id) == m_modelIds.end())
        {
            throw std::invalid_argument("modelrepository does not contain this modelID");
        }

        const auto offset{ m_modelToOffsetMap.at(id) };
        glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + offset));
        *modelMat = modelMatrix;
    }

    template<VertexDescription VD>
    void ModelRepository<VD>::translate(const ModelID id, const glm::vec3 & translate) const
    {
        if (m_modelIds.find(id) == m_modelIds.end())
        {
            throw std::invalid_argument("modelrepository does not contain this modelID");
        }

        const auto offset{ m_modelToOffsetMap.at(id) };
        glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + offset));
        *modelMat = glm::translate(*modelMat, translate);
    }

    template<VertexDescription VD>
    void ModelRepository<VD>::scale(const ModelID id, const glm::vec3 & scale) const
    {
        if (m_modelIds.find(id) == m_modelIds.end())
        {
            throw std::invalid_argument("modelrepository does not contain this modelID");
        }

        const auto offset{ m_modelToOffsetMap.at(id) };
        glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + offset));
        *modelMat = glm::scale(*modelMat, scale);
    }

    template<VertexDescription VD>
    void ModelRepository<VD>::rotate(const ModelID id, const glm::vec3 & axis, const float radians)
    {
        if (m_modelIds.find(id) == m_modelIds.end())
        {
            throw std::invalid_argument("modelrepository does not contain this modelID");
        }

        const auto offset{ m_modelToOffsetMap.at(id) };
        glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + offset));
        *modelMat = glm::rotate(*modelMat, radians, axis);
    }

    template<VertexDescription VD>
    vk::DescriptorBufferInfo ModelRepository<VD>::getDescriptorBufferInfo() const
    {
        return { *m_dynamicUniformBuffer, 0, sizeof(DynamicUniformBufferObject) };
    }

    template<VertexDescription VD>
    vk::WriteDescriptorSet ModelRepository<VD>::getWriteDescriptorSet(const vk::UniqueDescriptorSet & descriptorSet, const uint32_t binding, const vk::DescriptorBufferInfo & info) const
    {
        return { *descriptorSet, binding, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &info };
    }

    template<VertexDescription VD>
    void ModelRepository<VD>::flushDynamicBuffer(const vk::UniqueDevice & device) const
    {
        memcpy(m_mappedMemory, m_dynamicUniformBufferObject.model, m_bufferSize);
        vk::MappedMemoryRange mmr{ *m_dynamicUniformBufferMemory, 0, m_bufferSize };
        device->flushMappedMemoryRanges(mmr);
    }

    template<VertexDescription VD>
    void ModelRepository<VD>::draw(const vk::UniqueCommandBuffer & cmdBuffer, const vk::UniquePipelineLayout & pipelineLayout, const vk::UniqueDescriptorSet & descriptorSet) const
    {
        for (const auto & resourcePair : m_resourceMap)
        {
            const auto id{ resourcePair.first };
            const auto & resource{ resourcePair.second };
            resource.draw(m_dynamicOffsetMap.at(id), cmdBuffer, pipelineLayout, descriptorSet);
        }
    }

    template<VertexDescription VD>
    vk::DeviceSize ModelRepository<VD>::calculateDynamicAlignment(const vk::PhysicalDeviceProperties & properties, const vk::DeviceSize initialAlignment) const
    {
        const auto minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
        return minUboAlignment > 0 ? (initialAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1) : initialAlignment;
    }

    template<VertexDescription VD>
    ModelID ModelRepository<VD>::addInstance(const ModelResourceID & resourceId)
    {
        const ModelID modelId;
        const auto idx = *m_freeMatrixSpaces.begin();
        const auto dynamicOffset{ idx * m_dynamicAlignment };

        m_modelIds.emplace(modelId);

        // set initial matrix in dynamic UBO
        glm::mat4 * modelMat = reinterpret_cast<glm::mat4 *>((reinterpret_cast<uint64_t>(m_dynamicUniformBufferObject.model) + dynamicOffset));
        *modelMat = glm::mat4(1.f);

        // set dynamic offset
        m_modelToOffsetMap[modelId] = dynamicOffset;
        m_dynamicOffsetMap[resourceId].emplace(dynamicOffset);

        // set instanceId
        m_instanceMap[resourceId].emplace_back(modelId);

        m_freeMatrixSpaces.erase(idx);

        return modelId;
    }

    template class ModelRepository<VertexDescription::PositionNormalColor>;
    template class ModelRepository<VertexDescription::PositionNormalColorTexture>;
}
