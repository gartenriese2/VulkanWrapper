#include "surface.hpp"

namespace bmvk
{
    Surface::Surface(vk::UniqueSurfaceKHR && surface)
        : m_surface{std::move(surface)}
    {
    }
}