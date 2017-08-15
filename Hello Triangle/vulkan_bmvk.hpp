#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include "../extern/Vulkan-1.0.51.0/include/vulkan/vulkan.hpp"

namespace bmvk
{
    template <class UniqueHandle, class Handle, class Deleter>
    class OptRef
    {
    public:
        OptRef() : m_ref{} {}
        OptRef(UniqueHandle & uh) : m_ref{ uh } {}

        operator Handle()
        {
            if (!m_ref)
            {
                return nullptr;
            }

            auto & uniqueType = (*m_ref).get();
            return *uniqueType;
        }
    private:
        std::optional<std::reference_wrapper<UniqueHandle>> m_ref;

        static_assert(std::is_same_v<UniqueHandle, vk::UniqueHandle<Handle, Deleter>>);
    };

    using OptRefBuffer = OptRef<vk::UniqueBuffer, vk::Buffer, vk::BufferDeleter>;
    using OptRefBufferView = OptRef<vk::UniqueBufferView, vk::BufferView, vk::BufferViewDeleter>;
    using OptRefCommandBuffer = OptRef<vk::UniqueCommandBuffer, vk::CommandBuffer, vk::CommandBufferDeleter>;
    using OptRefCommandPool = OptRef<vk::UniqueCommandPool, vk::CommandPool, vk::CommandPoolDeleter>;
    using OptRefDescriptorPool = OptRef<vk::UniqueDescriptorPool, vk::DescriptorPool, vk::DescriptorPoolDeleter>;
    using OptRefDescriptorSet = OptRef<vk::UniqueDescriptorSet, vk::DescriptorSet, vk::DescriptorSetDeleter>;
    using OptRefDescriptorSetLayout = OptRef<vk::UniqueDescriptorSetLayout, vk::DescriptorSetLayout, vk::DescriptorSetLayoutDeleter>;
    using OptRefDescriptorUpdateTemplateKHR = OptRef<vk::UniqueDescriptorUpdateTemplateKHR, vk::DescriptorUpdateTemplateKHR, vk::DescriptorUpdateTemplateKHRDeleter>;
    using OptRefDeviceMemory = OptRef<vk::UniqueDeviceMemory, vk::DeviceMemory, vk::DeviceMemoryDeleter>;
    using OptRefEvent = OptRef<vk::UniqueEvent, vk::Event, vk::EventDeleter>;
    using OptRefFence = OptRef<vk::UniqueFence, vk::Fence, vk::FenceDeleter>;
    using OptRefFramebuffer = OptRef<vk::UniqueFramebuffer, vk::Framebuffer, vk::FramebufferDeleter>;
    using OptRefImage = OptRef<vk::UniqueImage, vk::Image, vk::ImageDeleter>;
    using OptRefImageView = OptRef<vk::UniqueImageView, vk::ImageView, vk::ImageViewDeleter>;
    using OptRefIndirectCommandsLayoutNVX = OptRef<vk::UniqueIndirectCommandsLayoutNVX, vk::IndirectCommandsLayoutNVX, vk::IndirectCommandsLayoutNVXDeleter>;
    using OptRefObjectTableNVX = OptRef<vk::UniqueObjectTableNVX, vk::ObjectTableNVX, vk::ObjectTableNVXDeleter>;
    using OptRefPipeline = OptRef<vk::UniquePipeline, vk::Pipeline, vk::PipelineDeleter>;
    using OptRefPipelineCache = OptRef<vk::UniquePipelineCache, vk::PipelineCache, vk::PipelineCacheDeleter>;
    using OptRefPipelineLayout = OptRef<vk::UniquePipelineLayout, vk::PipelineLayout, vk::PipelineLayoutDeleter>;
    using OptRefQueryPool = OptRef<vk::UniqueQueryPool, vk::QueryPool, vk::QueryPoolDeleter>;
    using OptRefRenderPass = OptRef<vk::UniqueRenderPass, vk::RenderPass, vk::RenderPassDeleter>;
    using OptRefSampler = OptRef<vk::UniqueSampler, vk::Sampler, vk::SamplerDeleter>;
    using OptRefSemaphore = OptRef<vk::UniqueSemaphore, vk::Semaphore, vk::SemaphoreDeleter>;
    using OptRefShaderModule = OptRef<vk::UniqueShaderModule, vk::ShaderModule, vk::ShaderModuleDeleter>;
    using OptRefSwapchainKHR = OptRef<vk::UniqueSwapchainKHR, vk::SwapchainKHR, vk::SwapchainKHRDeleter>;

    template <class T>
    struct VkStructBase
    {
        operator const T &() const noexcept
        {
            return m_internal;
        }

        bool operator==(const T & rhs) const
        {
            return m_internal == rhs.m_internal;
        }

        bool operator!=(const T & rhs) const
        {
            return !operator==(rhs);
        }
    protected:
        explicit VkStructBase(T && internal) : m_internal{ std::move(internal) } {}

        T m_internal;
    };

    struct CommandBufferAllocateInfo : VkStructBase<vk::CommandBufferAllocateInfo>
    {
        CommandBufferAllocateInfo(const vk::UniqueCommandPool & commandPool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary, uint32_t count = 0)
            : VkStructBase<vk::CommandBufferAllocateInfo>{ { *commandPool, level, count } }
        {
        }
    };

    struct PresentInfo : VkStructBase<vk::PresentInfoKHR>
    {
        PresentInfo(vk::ArrayProxy<vk::Semaphore> waitSemaphores = nullptr, vk::ArrayProxy<vk::SwapchainKHR> swapchains = nullptr, vk::ArrayProxy<uint32_t> imageIndices = nullptr, vk::ArrayProxy<vk::Result> results = nullptr)
            : VkStructBase<vk::PresentInfoKHR>{ { waitSemaphores.size(), waitSemaphores.data(), swapchains.size(), swapchains.data(), imageIndices.data(), results.data() } }
        {
        }
    };

    struct DescriptorPoolCreateInfo : VkStructBase<vk::DescriptorPoolCreateInfo>
    {
        DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags flags = vk::DescriptorPoolCreateFlags(), uint32_t maxSets = 0, vk::ArrayProxy<vk::DescriptorPoolSize> poolSizes = nullptr)
            : VkStructBase<vk::DescriptorPoolCreateInfo>{ { flags, maxSets, poolSizes.size(), poolSizes.data() } }
        {
        }
    };

    struct RenderPassCreateInfo : VkStructBase<vk::RenderPassCreateInfo>
    {
        RenderPassCreateInfo(vk::RenderPassCreateFlags flags = vk::RenderPassCreateFlags(), vk::ArrayProxy<vk::AttachmentDescription> attachments = nullptr, vk::ArrayProxy<vk::SubpassDescription> subpasses = nullptr, vk::ArrayProxy<vk::SubpassDependency> dependencies = nullptr)
            : VkStructBase<vk::RenderPassCreateInfo>{ { flags, attachments.size(), attachments.data(), subpasses.size(), subpasses.data(), dependencies.size(), dependencies.data() } }
        {
        }
    };

    struct SubmitInfo : VkStructBase<vk::SubmitInfo>
    {
        SubmitInfo(vk::ArrayProxy<vk::Semaphore> waitSemaphores, vk::PipelineStageFlags * waitDstStageFlags = nullptr, vk::ArrayProxy<vk::CommandBuffer> commandBuffers = nullptr, vk::ArrayProxy<vk::Semaphore> signalSemaphores = nullptr)
            : VkStructBase<vk::SubmitInfo>{ { waitSemaphores.size(), waitSemaphores.data(), waitDstStageFlags, commandBuffers.size(), commandBuffers.data(), signalSemaphores.size(), signalSemaphores.data() } }
        {
        }

        SubmitInfo(const vk::UniqueCommandBuffer & commandBuffer)
            : VkStructBase<vk::SubmitInfo>{ { 0, nullptr, nullptr, 1, &*commandBuffer } }
        {
        }

        SubmitInfo(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniqueSemaphore & waitSemaphore, const vk::UniqueSemaphore & signalSemaphore, vk::PipelineStageFlags flags)
            : VkStructBase<vk::SubmitInfo>{ { 1, &*waitSemaphore, &flags, 1, &*commandBuffer, 1, &*signalSemaphore} }
        {
        }
    };
}
