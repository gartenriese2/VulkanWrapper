#include "queue.hpp"

#include "commandbuffer.hpp"
#include "vulkan_bmvk.hpp"

namespace bmvk
{
    Queue::Queue(const vk::Queue & queue)
        : m_queue{queue}
    {
    }

    void Queue::submit(const CommandBuffer & cmdBuffer, vk::Fence fence) const
    {
        const SubmitInfo info(reinterpret_cast<const vk::UniqueCommandBuffer &>(cmdBuffer));
        const vk::SubmitInfo info_vk{ info };
        m_queue.submit(info_vk, fence);
    }
}


