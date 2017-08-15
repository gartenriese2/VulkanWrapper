#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>

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

    struct DescriptorBufferInfo : VkStructBase<vk::DescriptorBufferInfo>
    {
        DescriptorBufferInfo(const vk::UniqueBuffer & buffer, vk::DeviceSize offset = 0, vk::DeviceSize range = 0)
            : VkStructBase{ { *buffer, offset, range } }
        {
        }
    };

    struct SpecializationInfo : VkStructBase<vk::SpecializationInfo>
    {
        SpecializationInfo(vk::ArrayProxy<vk::SpecializationMapEntry> mapEntries = nullptr, size_t dataSize = 0, const void * data = nullptr)
            : VkStructBase{ { mapEntries.size(), mapEntries.data(), dataSize, data } }
        {
        }
    };

    struct ClearColorValue : VkStructBase<vk::ClearColorValue>
    {
        explicit ClearColorValue(float r, float g, float b, float a)
            : VkStructBase{ { std::array<float,4>{ {r, g, b, a} } } }
        {
        }

        explicit ClearColorValue(int32_t r, int32_t g, int32_t b, int32_t a)
            : VkStructBase{ { std::array<int32_t,4>{ {r, g, b, a} } } }
        {
        }

        explicit ClearColorValue(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
            : VkStructBase{ { std::array<uint32_t,4>{ {r, g, b, a} } } }
        {
        }
    };

    struct PresentRegion : VkStructBase<vk::PresentRegionKHR>
    {
        PresentRegion(vk::ArrayProxy<vk::RectLayerKHR> rectangles)
            : VkStructBase{ { rectangles.size(), rectangles.data() } }
        {
        }
    };

    struct DescriptorImageInfo : VkStructBase<vk::DescriptorImageInfo>
    {
        DescriptorImageInfo(const vk::UniqueSampler & sampler, const vk::UniqueImageView & imageView, vk::ImageLayout imageLayout = vk::ImageLayout::eUndefined)
            : VkStructBase{ { *sampler, *imageView, imageLayout } }
        {
        }
    };

    struct DeviceQueueCreateInfo : VkStructBase<vk::DeviceQueueCreateInfo>
    {
        DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags flags = {}, uint32_t queueFamilyIndex = 0, uint32_t queueCount = 0, vk::ArrayProxy<float> queuePriorities = nullptr)
            : VkStructBase{ { flags, queueFamilyIndex, queueCount, queuePriorities.data() } }
        {
        }
    };

    struct DeviceCreateInfo : VkStructBase<vk::DeviceCreateInfo>
    {
        DeviceCreateInfo(vk::DeviceCreateFlags flags = {}, vk::ArrayProxy<vk::DeviceQueueCreateInfo> queueCreateInfos = nullptr, vk::ArrayProxy<const char * const> enabledLayerNames = nullptr, vk::ArrayProxy<const char * const> enabledExtensionNames = nullptr, vk::ArrayProxy<vk::PhysicalDeviceFeatures> enabledFeatures = nullptr)
            : VkStructBase{ { flags, queueCreateInfos.size(), queueCreateInfos.data(), enabledLayerNames.size(), enabledLayerNames.data(), enabledExtensionNames.size(), enabledExtensionNames.data(), enabledFeatures.data() } }
        {
        }
    };

    struct InstanceCreateInfo : VkStructBase<vk::InstanceCreateInfo>
    {
        InstanceCreateInfo(vk::InstanceCreateFlags flags = vk::InstanceCreateFlags(), vk::ApplicationInfo * const applicationInfo = nullptr, vk::ArrayProxy<const char * const> enabledLayerNames = nullptr, vk::ArrayProxy<const char * const> enabledExtensionNames = nullptr)
            : VkStructBase{ { flags, applicationInfo, enabledLayerNames.size(), enabledLayerNames.data(), enabledExtensionNames.size(), enabledExtensionNames.data() } }
        {
        }
    };

    struct MappedMemoryRange : VkStructBase<vk::MappedMemoryRange>
    {
        MappedMemoryRange(const vk::UniqueDeviceMemory & memory, vk::DeviceSize size = 0, vk::DeviceSize offset = 0)
            : VkStructBase{ { *memory, offset, size } }
        {
        }
    };

    struct WriteDescriptorSet : VkStructBase<vk::WriteDescriptorSet>
    {
        WriteDescriptorSet(const vk::UniqueDescriptorSet & dstSet, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0, uint32_t descriptorCount = 0, vk::DescriptorType descriptorType = vk::DescriptorType::eSampler, const vk::DescriptorImageInfo * const imageInfo = nullptr, const vk::DescriptorBufferInfo * const bufferInfo = nullptr)
            : VkStructBase{ { *dstSet, dstBinding, dstArrayElement, descriptorCount, descriptorType, imageInfo, bufferInfo } }
        {
        }
    };

    // TODO

    struct CommandBufferAllocateInfo : VkStructBase<vk::CommandBufferAllocateInfo>
    {
        CommandBufferAllocateInfo(const vk::UniqueCommandPool & commandPool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary, uint32_t count = 0)
            : VkStructBase{ { *commandPool, level, count } }
        {
        }
    };

    struct PresentInfo : VkStructBase<vk::PresentInfoKHR>
    {
        PresentInfo(vk::ArrayProxy<vk::Semaphore> waitSemaphores = nullptr, vk::ArrayProxy<vk::SwapchainKHR> swapchains = nullptr, vk::ArrayProxy<uint32_t> imageIndices = nullptr, vk::ArrayProxy<vk::Result> results = nullptr)
            : VkStructBase{ { waitSemaphores.size(), waitSemaphores.data(), swapchains.size(), swapchains.data(), imageIndices.data(), results.data() } }
        {
        }
    };

    struct DescriptorPoolCreateInfo : VkStructBase<vk::DescriptorPoolCreateInfo>
    {
        DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags flags = vk::DescriptorPoolCreateFlags(), uint32_t maxSets = 0, vk::ArrayProxy<vk::DescriptorPoolSize> poolSizes = nullptr)
            : VkStructBase{ { flags, maxSets, poolSizes.size(), poolSizes.data() } }
        {
        }
    };

    struct RenderPassCreateInfo : VkStructBase<vk::RenderPassCreateInfo>
    {
        RenderPassCreateInfo(vk::RenderPassCreateFlags flags = vk::RenderPassCreateFlags(), vk::ArrayProxy<vk::AttachmentDescription> attachments = nullptr, vk::ArrayProxy<vk::SubpassDescription> subpasses = nullptr, vk::ArrayProxy<vk::SubpassDependency> dependencies = nullptr)
            : VkStructBase{ { flags, attachments.size(), attachments.data(), subpasses.size(), subpasses.data(), dependencies.size(), dependencies.data() } }
        {
        }
    };

    struct SubmitInfo : VkStructBase<vk::SubmitInfo>
    {
        SubmitInfo(vk::ArrayProxy<vk::Semaphore> waitSemaphores, vk::PipelineStageFlags * waitDstStageFlags = nullptr, vk::ArrayProxy<vk::CommandBuffer> commandBuffers = nullptr, vk::ArrayProxy<vk::Semaphore> signalSemaphores = nullptr)
            : VkStructBase{ { waitSemaphores.size(), waitSemaphores.data(), waitDstStageFlags, commandBuffers.size(), commandBuffers.data(), signalSemaphores.size(), signalSemaphores.data() } }
        {
        }

        SubmitInfo(const vk::UniqueCommandBuffer & commandBuffer)
            : VkStructBase{ { 0, nullptr, nullptr, 1, &*commandBuffer } }
        {
        }

        SubmitInfo(const vk::UniqueCommandBuffer & commandBuffer, const vk::UniqueSemaphore & waitSemaphore, const vk::UniqueSemaphore & signalSemaphore, vk::PipelineStageFlags flags)
            : VkStructBase{ { 1, &*waitSemaphore, &flags, 1, &*commandBuffer, 1, &*signalSemaphore} }
        {
        }
    };
}
