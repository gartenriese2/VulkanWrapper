#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>

namespace bmvk
{
    template <class UniqueHandle, class Handle>
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

        static_assert(std::is_same_v<UniqueHandle, vk::UniqueHandle<Handle>>);
    };

    using OptRefBuffer = OptRef<vk::UniqueBuffer, vk::Buffer>;
    using OptRefBufferView = OptRef<vk::UniqueBufferView, vk::BufferView>;
    using OptRefCommandBuffer = OptRef<vk::UniqueCommandBuffer, vk::CommandBuffer>;
    using OptRefCommandPool = OptRef<vk::UniqueCommandPool, vk::CommandPool>;
    using OptRefDescriptorPool = OptRef<vk::UniqueDescriptorPool, vk::DescriptorPool>;
    using OptRefDescriptorSet = OptRef<vk::UniqueDescriptorSet, vk::DescriptorSet>;
    using OptRefDescriptorSetLayout = OptRef<vk::UniqueDescriptorSetLayout, vk::DescriptorSetLayout>;
    using OptRefDescriptorUpdateTemplateKHR = OptRef<vk::UniqueDescriptorUpdateTemplateKHR, vk::DescriptorUpdateTemplateKHR>;
    using OptRefDeviceMemory = OptRef<vk::UniqueDeviceMemory, vk::DeviceMemory>;
    using OptRefEvent = OptRef<vk::UniqueEvent, vk::Event>;
    using OptRefFence = OptRef<vk::UniqueFence, vk::Fence>;
    using OptRefFramebuffer = OptRef<vk::UniqueFramebuffer, vk::Framebuffer>;
    using OptRefImage = OptRef<vk::UniqueImage, vk::Image>;
    using OptRefImageView = OptRef<vk::UniqueImageView, vk::ImageView>;
    using OptRefIndirectCommandsLayoutNVX = OptRef<vk::UniqueIndirectCommandsLayoutNVX, vk::IndirectCommandsLayoutNVX>;
    using OptRefObjectTableNVX = OptRef<vk::UniqueObjectTableNVX, vk::ObjectTableNVX>;
    using OptRefPipeline = OptRef<vk::UniquePipeline, vk::Pipeline>;
    using OptRefPipelineCache = OptRef<vk::UniquePipelineCache, vk::PipelineCache>;
    using OptRefPipelineLayout = OptRef<vk::UniquePipelineLayout, vk::PipelineLayout>;
    using OptRefQueryPool = OptRef<vk::UniqueQueryPool, vk::QueryPool>;
    using OptRefRenderPass = OptRef<vk::UniqueRenderPass, vk::RenderPass>;
    using OptRefSampler = OptRef<vk::UniqueSampler, vk::Sampler>;
    using OptRefSemaphore = OptRef<vk::UniqueSemaphore, vk::Semaphore>;
    using OptRefShaderModule = OptRef<vk::UniqueShaderModule, vk::ShaderModule>;
    using OptRefSwapchainKHR = OptRef<vk::UniqueSwapchainKHR, vk::SwapchainKHR>;

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

    struct CopyDescriptorSet : VkStructBase<vk::CopyDescriptorSet>
    {
        CopyDescriptorSet(const vk::UniqueDescriptorSet & srcSet, uint32_t srcBinding, uint32_t srcArrayElement, const vk::UniqueDescriptorSet & dstSet, uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount)
            : VkStructBase{ { *srcSet, srcBinding, srcArrayElement, *dstSet, dstBinding, dstArrayElement, descriptorCount } }
        {
        }
    };

    struct BufferViewCreateInfo : VkStructBase<vk::BufferViewCreateInfo>
    {
        BufferViewCreateInfo(vk::BufferViewCreateFlags flags, const vk::UniqueBuffer & buffer, vk::Format format = vk::Format::eUndefined, vk::DeviceSize offset = 0, vk::DeviceSize range = 0)
            : VkStructBase{ { flags, *buffer, format, offset, range } }
        {
        }
    };

    struct ShaderModuleCreateInfo : VkStructBase<vk::ShaderModuleCreateInfo>
    {
        ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags flags, vk::ArrayProxy<uint32_t> code)
            : VkStructBase{ { flags, code.size(), code.data() } }
        {
        }

        ShaderModuleCreateInfo(vk::ArrayProxy<uint32_t> code)
            : VkStructBase{ { {}, code.size(), code.data() } }
        {
        }

        ShaderModuleCreateInfo(vk::ArrayProxy<const char> code)
            : VkStructBase{ { {}, code.size(), reinterpret_cast<const uint32_t *>(code.data()) } }
        {
        }
    };

    struct DescriptorSetAllocateInfo : VkStructBase<vk::DescriptorSetAllocateInfo>
    {
        DescriptorSetAllocateInfo(const vk::UniqueDescriptorPool & descriptorPool, vk::ArrayProxy<vk::DescriptorSetLayout> setLayouts)
            : VkStructBase{ { *descriptorPool, setLayouts.size(), setLayouts.data() } }
        {
        }
    };

    struct PipelineVertexInputStateCreateInfo : VkStructBase<vk::PipelineVertexInputStateCreateInfo>
    {
        PipelineVertexInputStateCreateInfo(vk::ArrayProxy<vk::VertexInputBindingDescription> vertexBindingDescriptions, vk::ArrayProxy<vk::VertexInputAttributeDescription> vertexAttributeDescriptions)
            : VkStructBase{ { {}, vertexBindingDescriptions.size(), vertexBindingDescriptions.data(), vertexAttributeDescriptions.size(), vertexAttributeDescriptions.data() } }
        {
        }
    };

    struct PipelineInputAssemblyStateCreateInfo : VkStructBase<vk::PipelineInputAssemblyStateCreateInfo>
    {
        PipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology topology = vk::PrimitiveTopology::ePointList, bool primitiveRestartEnable = false)
            : VkStructBase{ { {}, topology, primitiveRestartEnable } }
        {
        }
    };

    struct PipelineTessellationStateCreateInfo : VkStructBase<vk::PipelineTessellationStateCreateInfo>
    {
        PipelineTessellationStateCreateInfo(uint32_t patchControlPoints = 0)
            : VkStructBase{ { {}, patchControlPoints } }
        {
        }
    };

    struct PipelineViewportStateCreateInfo : VkStructBase<vk::PipelineViewportStateCreateInfo>
    {
        PipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags flags, vk::ArrayProxy<vk::Viewport> viewports, vk::ArrayProxy<vk::Rect2D> scissors)
            : VkStructBase{ { flags, viewports.size(), viewports.data(), scissors.size(), scissors.data() } }
        {
        }

        PipelineViewportStateCreateInfo(vk::ArrayProxy<vk::Viewport> viewports, vk::ArrayProxy<vk::Rect2D> scissors)
            : VkStructBase{ { {}, viewports.size(), viewports.data(), scissors.size(), scissors.data() } }
        {
        }
    };

    struct CommandBufferAllocateInfo : VkStructBase<vk::CommandBufferAllocateInfo>
    {
        CommandBufferAllocateInfo(const vk::UniqueCommandPool & commandPool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary, uint32_t count = 0)
            : VkStructBase{ { *commandPool, level, count } }
        {
        }
    };

    struct RenderPassBeginInfo : VkStructBase<vk::RenderPassBeginInfo>
    {
        RenderPassBeginInfo(const vk::UniqueRenderPass & renderpass, const vk::UniqueFramebuffer & framebuffer, vk::Rect2D renderArea, vk::ArrayProxy<vk::ClearValue> clearValues)
            : VkStructBase{ { *renderpass, *framebuffer, renderArea, clearValues.size(), clearValues.data() } }
        {
        }
    };

    // TODO

    struct DescriptorSetLayoutCreateInfo : VkStructBase<vk::DescriptorSetLayoutCreateInfo>
    {
        DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags flags, vk::ArrayProxy<vk::DescriptorSetLayoutBinding> bindings)
            : VkStructBase{ { flags, bindings.size(), bindings.data() } }
        {
        }

        DescriptorSetLayoutCreateInfo(const std::vector<vk::DescriptorSetLayoutBinding> & bindings)
            : VkStructBase{ { {}, static_cast<uint32_t>(bindings.size()), bindings.data() } }
        {
        }
    };

    struct PipelineColorBlendStateCreateInfo : VkStructBase<vk::PipelineColorBlendStateCreateInfo>
    {
        PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlags flags, bool logicOpEnable, vk::LogicOp logicOp, vk::ArrayProxy<vk::PipelineColorBlendAttachmentState> attachments, const std::array<float, 4> & blendConstants = { { 0, 0, 0, 0 } })
            : VkStructBase{ { flags, logicOpEnable, logicOp, attachments.size(), attachments.data(), blendConstants } }
        {
        }

        PipelineColorBlendStateCreateInfo(bool logicOpEnable, vk::LogicOp logicOp, vk::ArrayProxy<vk::PipelineColorBlendAttachmentState> attachments, const std::array<float, 4> & blendConstants = { { 0, 0, 0, 0 } })
            : VkStructBase{ { {}, logicOpEnable, logicOp, attachments.size(), attachments.data(), blendConstants } }
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

    struct PipelineDynamicStateCreateInfo : VkStructBase<vk::PipelineDynamicStateCreateInfo>
    {
        PipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlags flags, vk::ArrayProxy<const vk::DynamicState> dynamicStates)
            : VkStructBase{ { flags, dynamicStates.size(), dynamicStates.data() } }
        {
        }

        PipelineDynamicStateCreateInfo(vk::ArrayProxy<const vk::DynamicState> dynamicStates)
            : VkStructBase{ { {}, dynamicStates.size(), dynamicStates.data() } }
        {
        }
    };

    struct PipelineLayoutCreateInfo : VkStructBase<vk::PipelineLayoutCreateInfo>
    {
        PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags flags, vk::ArrayProxy<vk::DescriptorSetLayout> setLayouts, vk::ArrayProxy<vk::PushConstantRange> pushConstantRanges)
            : VkStructBase{ { flags, setLayouts.size(), setLayouts.data(), pushConstantRanges.size(), pushConstantRanges.data() } }
        {
        }

        PipelineLayoutCreateInfo(const std::vector<vk::DescriptorSetLayout> & setLayouts, const std::vector<vk::PushConstantRange> & pushConstantRanges = {})
            : VkStructBase{ { {}, static_cast<uint32_t>(setLayouts.size()), setLayouts.data(), static_cast<uint32_t>(pushConstantRanges.size()), pushConstantRanges.data() } }
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
