#include "surface.hpp"

namespace bmvk
{
    Surface::Surface(vk::SurfaceKHR surface)
        : m_surface{std::move(surface)}
    {
    }

    Surface::~Surface()
    {

    }

}