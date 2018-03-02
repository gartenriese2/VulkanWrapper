#include "demo.hpp"

#include <iostream>

namespace bmvk
{
    template <vw::scene::VertexDescription VD>
    Demo<VD>::Demo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name, const DebugReport::ReportLevel reportLevel, const uint32_t maxModelRepositoryInstances)
      : m_window{ width, height, name },
        m_instance{ name, VK_MAKE_VERSION(1, 0, 0), "bmvk", VK_MAKE_VERSION(1, 0, 0), m_window, enableValidationLayers, reportLevel },
        m_device{ m_instance.getPhysicalDevice().createLogicalDevice(m_instance.getLayerNames()) },
        m_queue{ m_device.createQueue() },
        m_commandPool{ m_device.createCommandPool() },
        m_bufferFactory{ m_device, static_cast<vk::PhysicalDevice>(m_instance.getPhysicalDevice()) },
        m_modelRepository{ reinterpret_cast<const vk::UniqueDevice &>(m_device), reinterpret_cast<const vk::PhysicalDevice &>(m_instance.getPhysicalDevice()), maxModelRepositoryInstances },
        m_timepoint{ std::chrono::steady_clock::now() },
        m_timepointCount{ 0 },
        m_elapsedTime{ std::chrono::microseconds::zero() }
    {
    }

    template <vw::scene::VertexDescription VD>
    void Demo<VD>::copyBuffer(vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const
    {
        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmdBuffer.copyBuffer(srcBuffer, dstBuffer, size);
        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_queue.waitIdle();
    }

    template <vw::scene::VertexDescription VD>
    void Demo<VD>::copyBufferToImage(vk::UniqueBuffer & buffer, vk::UniqueImage & image, uint32_t width, uint32_t height) const
    {
        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        vk::BufferImageCopy region{ 0, 0, 0, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, {}, { width, height, 1 } };
        cmdBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_queue.waitIdle();
    }

    template <vw::scene::VertexDescription VD>
    void Demo<VD>::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory) const
    {
        vk::BufferCreateInfo bufferInfo{ {}, size, usage };
        buffer = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createBufferUnique(bufferInfo);

        const auto memRequirements{ reinterpret_cast<const vk::UniqueDevice &>(m_device)->getBufferMemoryRequirements(*buffer) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, m_instance.getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, properties) };
        bufferMemory = reinterpret_cast<const vk::UniqueDevice &>(m_device)->allocateMemoryUnique(allocInfo);

        reinterpret_cast<const vk::UniqueDevice &>(m_device)->bindBufferMemory(*buffer, *bufferMemory, 0);
    }

    template <vw::scene::VertexDescription VD>
    void Demo<VD>::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueImage & image, vk::UniqueDeviceMemory & imageMemory)
    {
        vk::ImageCreateInfo imageInfo{ {}, vk::ImageType::e2D, format, { width, height, 1 }, 1, 1, vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive };
        image = reinterpret_cast<const vk::UniqueDevice &>(m_device)->createImageUnique(imageInfo);

        const auto memRequirements{ reinterpret_cast<const vk::UniqueDevice &>(m_device)->getImageMemoryRequirements(*image) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, m_instance.getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, properties) };
        imageMemory = reinterpret_cast<const vk::UniqueDevice &>(m_device)->allocateMemoryUnique(allocInfo);

        reinterpret_cast<const vk::UniqueDevice &>(m_device)->bindImageMemory(*image, *imageMemory, 0);
    }

    template <vw::scene::VertexDescription VD>
    vk::UniqueImageView Demo<VD>::createImageView(const vk::UniqueImage & image, vk::Format format, vk::ImageAspectFlags aspectFlags) const
    {
        vk::ImageViewCreateInfo viewInfo{ {}, *image, vk::ImageViewType::e2D, format, {} ,{ aspectFlags, 0, 1, 0, 1 } };
        return m_device.createImageView(viewInfo);
    }

    template <vw::scene::VertexDescription VD>
    bool Demo<VD>::hasStencilComponent(const vk::Format format) const
    {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    template <vw::scene::VertexDescription VD>
    void Demo<VD>::transitionImageLayout(const CommandBuffer & cmdBuffer, const vk::UniqueImage & image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const
    {
        vk::ImageMemoryBarrier barrier{ {},{}, oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *image, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
        vk::PipelineStageFlags srcStage;
        vk::PipelineStageFlags dstStage;

        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
            if (hasStencilComponent(format))
            {
                barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        }

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
            dstStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            srcStage = vk::PipelineStageFlagBits::eTransfer;
            dstStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
            srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
            dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }
        
        cmdBuffer.pipelineBarrier(srcStage, dstStage, {}, nullptr, nullptr, barrier);
    }

    template <vw::scene::VertexDescription VD>
    void Demo<VD>::timing(const bool print)
    {
        const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_timepoint);
        m_currentFrameTime = microseconds.count() / 1000000.0;
        m_elapsedTime += microseconds;
        ++m_timepointCount;
        if (m_elapsedTime.count() > 1e6)
        {
            m_avgFrameTime = static_cast<double>(m_elapsedTime.count()) / static_cast<double>(m_timepointCount);
            m_avgFps = 1e6 / m_avgFrameTime;
            if (print)
            {
                std::cout << "Avg frametime: " << m_avgFrameTime << " microseconds. Avg FPS: " << m_avgFps << " fps\n";
            }

            m_elapsedTime = std::chrono::microseconds::zero();
            m_timepointCount = 0;
        }

        m_timepoint = std::chrono::steady_clock::now();
    }

    template class Demo<vw::scene::VertexDescription::NotUsed>;
    template class Demo<vw::scene::VertexDescription::PositionNormalColor>;
    template class Demo<vw::scene::VertexDescription::PositionNormalColorTexture>;
}
