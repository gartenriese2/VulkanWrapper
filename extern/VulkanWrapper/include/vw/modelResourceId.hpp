#pragma once

#include <cstdint>
#include <functional>

namespace vw::scene
{
    class ModelResourceID
    {
    public:
        ModelResourceID()
        {
            static uint64_t s_nextID = 1;
            m_id = s_nextID++;
            if (s_nextID == 0)
            {
                throw std::runtime_error("Maximum number of ModelResources reached");
            }
        }

        bool operator==(const ModelResourceID & rhs) const { return m_id == rhs.m_id; }

        struct KeyHash
        {
            std::size_t operator()(const ModelResourceID & k) const
            {
                return std::hash<uint64_t>()(k.m_id);
            }
        };
    private:
        uint64_t m_id;
    };
}
