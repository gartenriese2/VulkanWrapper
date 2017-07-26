#pragma once
#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class Queue
    {
    public:
        explicit Queue(const vk::Queue & queue);
        ~Queue();
    private:
        vk::Queue m_queue;
    };
}



