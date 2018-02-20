#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp>

#include <type_traits>

namespace vw::scene
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            return { 0, sizeof Vertex, vk::VertexInputRate::eVertex };
        }

        static auto getAttributeDescriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions =
            {
                vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) },
                vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) },
                vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) },
                vk::VertexInputAttributeDescription{ 3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) }
            };
            return attributeDescriptions;
        }

        auto operator==(const Vertex & other) const
        {
            return pos == other.pos && normal == other.normal && color == other.color && texCoord == other.texCoord;
        }
    };

    

    static_assert(std::is_nothrow_move_constructible_v<Vertex>);
    static_assert(std::is_nothrow_copy_constructible_v<Vertex>);
    static_assert(std::is_nothrow_move_assignable_v<Vertex>);
    static_assert(std::is_nothrow_copy_assignable_v<Vertex>);
}

namespace std
{
    template<> struct hash<vw::scene::Vertex>
    {
        size_t operator()(vw::scene::Vertex const & vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1) ^ hash<glm::vec3>()(vertex.normal);
        }
    };
}
