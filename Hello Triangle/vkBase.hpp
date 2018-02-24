#pragma once

template <typename T>
class VkBase
{
public:
    explicit operator const T &() const noexcept { return m_vkType; }
protected:
    T m_vkType;
};