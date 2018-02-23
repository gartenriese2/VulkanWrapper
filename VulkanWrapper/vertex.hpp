#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp>

#include <type_traits>

namespace vw::scene
{
    enum class VertexDescription
    {
        PositionNormalColorTexture,
        PositionNormalColor
    };

    template<VertexDescription VD>
    struct Vertex
    {
    };

    template<>
    struct Vertex<VertexDescription::PositionNormalColorTexture>
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            return { 0, sizeof (Vertex<VertexDescription::PositionNormalColorTexture>), vk::VertexInputRate::eVertex };
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

    static_assert(std::is_nothrow_move_constructible_v<Vertex<VertexDescription::PositionNormalColorTexture>>);
    static_assert(std::is_nothrow_copy_constructible_v<Vertex<VertexDescription::PositionNormalColorTexture>>);
    static_assert(std::is_nothrow_move_assignable_v<Vertex<VertexDescription::PositionNormalColorTexture>>);
    static_assert(std::is_nothrow_copy_assignable_v<Vertex<VertexDescription::PositionNormalColorTexture>>);

    template<>
    struct Vertex<VertexDescription::PositionNormalColor>
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            return { 0, sizeof (Vertex<VertexDescription::PositionNormalColor>), vk::VertexInputRate::eVertex };
        }

        static auto getAttributeDescriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions =
            {
                vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) },
                vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) },
                vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) }
            };
            return attributeDescriptions;
        }

        auto operator==(const Vertex & other) const
        {
            return pos == other.pos && normal == other.normal && color == other.color;
        }
    };

    static_assert(std::is_nothrow_move_constructible_v<Vertex<VertexDescription::PositionNormalColor>>);
    static_assert(std::is_nothrow_copy_constructible_v<Vertex<VertexDescription::PositionNormalColor>>);
    static_assert(std::is_nothrow_move_assignable_v<Vertex<VertexDescription::PositionNormalColor>>);
    static_assert(std::is_nothrow_copy_assignable_v<Vertex<VertexDescription::PositionNormalColor>>);
}

namespace std
{
    template<vw::scene::VertexDescription VD> struct hash<vw::scene::Vertex<VD>>
    {
        template <vw::scene::VertexDescription vd = VD>
        size_t operator()(vw::scene::Vertex<VD> const & vertex, typename std::enable_if_t<vd == vw::scene::VertexDescription::PositionNormalColorTexture> * = nullptr) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1) ^ hash<glm::vec3>()(vertex.normal);
        }

        template <vw::scene::VertexDescription vd = VD>
        size_t operator()(vw::scene::Vertex<VD> const & vertex, typename std::enable_if_t<vd == vw::scene::VertexDescription::PositionNormalColor> * = nullptr) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ hash<glm::vec3>()(vertex.normal);
        }
    };
}
