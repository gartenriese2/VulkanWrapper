#include "queue.hpp"

#include "commandbuffer.hpp"
#include "vulkan_bmvk.hpp"

namespace bmvk
{
    Queue::Queue(vk::Queue && queue)
        : m_queue{std::move(queue)}
    {
    }

    void Queue::submit(const CommandBuffer & cmdBuffer, vk::Fence fence) const
    {
        const SubmitInfo info{ reinterpret_cast<const vk::UniqueCommandBuffer &>(cmdBuffer) };
        const vk::SubmitInfo info_vk{ info };
        m_queue.submit(info_vk, fence);
    }

    void Queue::submit(const CommandBuffer & cmdBuffer, const vk::UniqueSemaphore & waitSemaphore, const vk::UniqueSemaphore & signalSemaphore, vk::PipelineStageFlags flags, vk::Fence fence) const
    {
        vk::Semaphore waitSemaphores[] = { *waitSemaphore };
        vk::PipelineStageFlags waitStages[] = { flags };
        const auto & uniqueCmdBuffer{ reinterpret_cast<const vk::UniqueCommandBuffer &>(cmdBuffer) };
        vk::CommandBuffer commandBuffers[] = { *uniqueCmdBuffer };
        vk::Semaphore signalSemaphores[] = { *signalSemaphore };
        vk::SubmitInfo info_vk{ 1, waitSemaphores, waitStages, 1, commandBuffers, 1, signalSemaphores };
        m_queue.submit(info_vk, fence);
    }

    bool Queue::present(vk::ArrayProxy<vk::Semaphore> waitSemaphores, vk::ArrayProxy<vk::SwapchainKHR> swapchains, vk::ArrayProxy<uint32_t> imageIndices, vk::ArrayProxy<vk::Result> results) const
    {
        try
        {
            PresentInfo info{ waitSemaphores, swapchains, imageIndices, results };
            return m_queue.presentKHR(info) != vk::Result::eSuboptimalKHR;
        }
        catch (const vk::OutOfDateKHRError &)
        {
            return false;
        }
    }
}


