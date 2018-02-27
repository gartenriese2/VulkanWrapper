#pragma once

#include <cstdint>

namespace vw::scene
{
    class ModelID
    {
    public:
        ModelID()
        {
            static uint64_t s_nextID = 1;
            m_id = s_nextID++;
        }

        bool operator==(const ModelID & rhs) const { return m_id == rhs.m_id; }

        struct KeyHash
        {
            std::size_t operator()(const ModelID & k) const
            {
                return std::hash<uint64_t>()(k.m_id);
            }
        };
    private:
        uint64_t m_id;
    };

    
}