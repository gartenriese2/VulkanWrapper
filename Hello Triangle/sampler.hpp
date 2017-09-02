#pragma once

#include <type_traits>

#include "vulkan_bmvk.hpp"

namespace bmvk
{
    class Sampler
    {
    public:
        Sampler(const vk::Device & device, const vk::Filter magFilter, const vk::Filter minFilter, const vk::SamplerMipmapMode mipmapMode, const vk::SamplerAddressMode addressModeU, const vk::SamplerAddressMode addressModeV, const vk::SamplerAddressMode addressModeW, const float mipLodBias, const bool enableAnisotropy, const float maxAnisotropy, const bool enableCompare, const vk::CompareOp compareOp, const float minLod, const float maxLod, const vk::BorderColor borderColor, const bool useUnnormalizedCoordinates);
        Sampler(const Sampler &) = delete;
        Sampler(Sampler && other) = default;
        Sampler & operator=(const Sampler &) = delete;
        Sampler & operator=(Sampler && other) = default;
        ~Sampler() {}

        explicit operator const vk::UniqueSampler &() const noexcept { return m_sampler; }
    private:
        vk::UniqueSampler m_sampler;
    };

    static_assert(std::is_move_constructible_v<Sampler>);
    static_assert(std::is_move_assignable_v<Sampler>);
    static_assert(!std::is_copy_constructible_v<Sampler>);
    static_assert(!std::is_copy_assignable_v<Sampler>);
}