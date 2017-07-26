#include "queue.hpp"

namespace bmvk
{
    Queue::Queue(const vk::Queue & queue)
        : m_queue{queue}
    {
    }

    Queue::~Queue()
    {
    }
}


