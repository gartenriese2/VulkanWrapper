#pragma once

#include "vertex.hpp"
#include "model.hpp"
#include "modelGroup.hpp"

namespace vw::scene
{
    template<VertexDescription VD>
    class Scene
    {
    public:
        auto & getModel(ModelID id)
        {
            auto it = m_models.find(id);
            if (it == m_models.end())
            {
                throw std::invalid_argument("No model with this id exists");
            }

            return *it;
        }

        auto addModel(Model<VD> && model)
        {
            ModelID id;
            m_models.emplace(id, std::move(model));
            return id;
        }

        auto & getModelGroup(ModelID id)
        {
            auto it = std::find_if(m_modelGroups.begin(), m_modelGroups.end(), [id](const auto & groupPair)
            {
                return groupPair.key.find(id) != groupPair.key.end();
            });

            if (it == m_modelGroups.end())
            {
                throw std::invalid_argument("No modelgroup that contains this id");
            }

            return *it;
        }
    private:
        std::unordered_map<ModelID, Model<VD>> m_models;
        std::unordered_map<std::vector<ModelID>, ModelGroup<VD>> m_modelGroups;
    };
}
