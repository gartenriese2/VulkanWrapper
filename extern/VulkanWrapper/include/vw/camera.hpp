#pragma once

#include <glm/gtc/quaternion.hpp>

#include <limits>
#include <type_traits>

namespace vw::util
{
    class Camera
    {
    public:
        Camera();
        Camera(const glm::vec3 & pos, const glm::vec3 & dir, const glm::vec3 & up,
               const float fov, const float ratio, const float near, const float far = std::numeric_limits<float>::infinity());

        const glm::mat4 & getViewMatrix() const;
        const glm::mat4 & getProjMatrix() const;

        void setFov(float val) noexcept;
        void setRatio(float val) noexcept;
        void setNear(float val) noexcept;
        void setFar(float val) noexcept;

        void roll(float angle); // rotate around z-axis
        void pitch(float angle); // rotate around x-axis
        void yaw(float angle); // rotate around y-axis
        void rotate(float angle, const glm::vec3 & axis);
        void translate(const glm::vec3 & v);
        void translateLocal(const glm::vec3 & v);
    private:
        glm::vec3 m_pos;
        glm::vec3 m_dir;
        glm::vec3 m_up;
        glm::quat m_orientation;

        float m_fov;
        float m_ratio;
        float m_near;
        float m_far;

        mutable glm::mat4 m_viewMat;
        mutable glm::mat4 m_projMat;

        mutable bool m_modified;
        mutable bool m_modifiedProj;
    };

    static_assert(std::is_nothrow_move_constructible_v<Camera>);
    static_assert(std::is_nothrow_copy_constructible_v<Camera>);
    static_assert(std::is_nothrow_move_assignable_v<Camera>);
    static_assert(std::is_nothrow_copy_assignable_v<Camera>);
}
