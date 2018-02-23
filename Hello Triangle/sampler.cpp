#include "sampler.hpp"

namespace bmvk
{
    Sampler::Sampler(const vk::UniqueDevice & device, const vk::Filter magFilter, const vk::Filter minFilter, const vk::SamplerMipmapMode mipmapMode, const vk::SamplerAddressMode addressModeU, const vk::SamplerAddressMode addressModeV, const vk::SamplerAddressMode addressModeW, const float mipLodBias, const bool enableAnisotropy, const float maxAnisotropy, const bool enableCompare, const vk::CompareOp compareOp, const float minLod, const float maxLod, const vk::BorderColor borderColor, const bool useUnnormalizedCoordinates)
    {
        vk::SamplerCreateInfo info{ {}, magFilter, minFilter, mipmapMode, addressModeU, addressModeV, addressModeW, mipLodBias, enableAnisotropy, maxAnisotropy, enableCompare, compareOp, minLod, maxLod, borderColor, useUnnormalizedCoordinates };
        m_sampler = device->createSamplerUnique(info);
    }
}