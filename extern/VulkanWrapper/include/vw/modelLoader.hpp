#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <type_traits>

#include "model.hpp"

namespace vw::scene
{
    template<VertexDescription VD>
    class ModelLoader
    {
    public:
        enum class NormalCreation
        {
            AssimpNormals,
            AssimpSmoothNormals,
            Explicit
        };

        template <VertexDescription vd = VD>
        auto createVertex(const glm::vec3 & v, const glm::vec3 & n, const glm::vec3 & c, const glm::vec2 & t, typename std::enable_if_t<vd == VertexDescription::PositionNormalColorTexture> * = nullptr) const
        {
            return Vertex<VD>{ v, n, c, t };
        }

        template <VertexDescription vd = VD>
        auto createVertex(const glm::vec3 & v, const glm::vec3 & n, const glm::vec3 & c, const glm::vec2 & t, typename std::enable_if_t<vd == VertexDescription::PositionNormalColor> * = nullptr) const
        {
            return Vertex<VD>{ v, n, c };
        }

        Model<VD> loadModel(std::string_view file, const NormalCreation normalCreation)
        {
            Model<VD> model;
            model.getVertices().clear();
            model.getIndices().clear();

            int aiProcessFlags{ aiProcess_Triangulate };
            if (normalCreation == NormalCreation::AssimpNormals)
            {
                aiProcessFlags |= aiProcess_GenNormals;
            }
            else if (normalCreation == NormalCreation::AssimpSmoothNormals)
            {
                aiProcessFlags |= aiProcess_GenSmoothNormals;
            }

            const auto * scene = m_importer.ReadFile(file.data(), aiProcessFlags);
            if (!scene)
            {
                throw std::runtime_error(m_importer.GetErrorString());
            }

            std::unordered_map<Vertex<VD>, uint32_t> uniqueVertices = {};

            const auto meshes = scene->mMeshes;
            const auto numMeshes = scene->mNumMeshes;
            for (uint32_t i = 0; i < numMeshes; ++i)
            {
                const auto mesh = meshes[i];
                const auto vertices = mesh->mVertices;
                const auto normals = mesh->mNormals;
                const auto faces = mesh->mFaces;
                const auto numVertices = mesh->mNumVertices;
                const auto numFaces = mesh->mNumFaces;

                for (uint32_t j = 0; j < numFaces; ++j)
                {
                    const auto face = faces[j];
                    const auto indices = face.mIndices;
                    const auto numIndices = face.mNumIndices;
                    if (numIndices != 3)
                    {
                        throw std::runtime_error("no triangles");
                    }
                    
                    glm::vec3 n;
                    if (normalCreation == NormalCreation::Explicit)
                    {
                        const auto a_assimp = vertices[indices[0]];
                        const auto a = glm::vec3(a_assimp.x, a_assimp.y, a_assimp.z);
                        const auto b_assimp = vertices[indices[1]];
                        const auto b = glm::vec3(b_assimp.x, b_assimp.y, b_assimp.z);
                        const auto c_assimp = vertices[indices[2]];
                        const auto c = glm::vec3(c_assimp.x, c_assimp.y, c_assimp.z);
                        n = glm::normalize(glm::cross(b - a, c - a));
                    }

                    for (uint32_t k = 0; k < numIndices; ++k)
                    {
                        const auto index = indices[k];
                        if (index >= numVertices)
                        {
                            throw std::runtime_error("index too big");
                        }

                        const auto v = vertices[index];
                        if (normalCreation == NormalCreation::AssimpNormals || normalCreation == NormalCreation::AssimpSmoothNormals)
                        {
                            const auto aiN{ normals[index] };
                            n = { aiN.x, aiN.y, aiN.z };
                        }

                        const auto vertex{ createVertex<VD>({ v.x, v.y, v.z }, n, { 1.f, 0.f, 0.f }, { 0.f, 1.f }) };

                        if (uniqueVertices.count(vertex) == 0)
                        {
                            uniqueVertices[vertex] = static_cast<uint32_t>(model.getVertices().size());
                            model.getVertices().push_back(vertex);
                        }

                        model.getIndices().push_back(uniqueVertices[vertex]);
                    }
                }
            }

            return model;
        }

        Model<VD> loadTriangle() const
        {
            Model<VD> model;
            auto & modelVertices{ model.getVertices() };
            auto & modelIndices{ model.getIndices() };
            modelVertices.clear();
            modelIndices.clear();

            Vertex<VD> v1;
            v1.pos = { -1.f, -1.f, 0.f };
            v1.texCoord = { 0.f, 1.f };
            v1.color = { 1.f, 0.f, 0.f };
            v1.normal = { 0.f, 0.f, 1.f };
            Vertex<VD> v2;
            v2.pos = { 1.f, -1.f, 0.f };
            v2.texCoord = { 0.f, 1.f };
            v2.color = { 0.f, 1.f, 0.f };
            v2.normal = { 0.f, 0.f, 1.f };
            Vertex<VD> v3;
            v3.pos = { 0.f, 1.f, 0.f };
            v3.texCoord = { 0.f, 1.f };
            v3.color = { 0.f, 0.f, 1.f };
            v3.normal = { 0.f, 0.f, 1.f };

            modelVertices.push_back(v1);
            modelVertices.push_back(v2);
            modelVertices.push_back(v3);

            modelIndices.push_back(0);
            modelIndices.push_back(1);
            modelIndices.push_back(2);

            return model;
        }
    private:
        Assimp::Importer m_importer;
    };

    static_assert(std::is_move_constructible_v<ModelLoader<VertexDescription::PositionNormalColorTexture>>);
    static_assert(std::is_copy_constructible_v<ModelLoader<VertexDescription::PositionNormalColorTexture>>);
    static_assert(std::is_nothrow_move_assignable_v<ModelLoader<VertexDescription::PositionNormalColorTexture>>);
    static_assert(std::is_nothrow_copy_assignable_v<ModelLoader<VertexDescription::PositionNormalColorTexture>>);

    static_assert(std::is_move_constructible_v<ModelLoader<VertexDescription::PositionNormalColor>>);
    static_assert(std::is_copy_constructible_v<ModelLoader<VertexDescription::PositionNormalColor>>);
    static_assert(std::is_nothrow_move_assignable_v<ModelLoader<VertexDescription::PositionNormalColor>>);
    static_assert(std::is_nothrow_copy_assignable_v<ModelLoader<VertexDescription::PositionNormalColor>>);
}