#ifndef AABB_SDF_H
#define AABB_SDF_H

#include "signed_distance_field.h"

namespace godot
{
class JarBoxSdf : public JarSignedDistanceField
{
    GDCLASS(JarBoxSdf, JarSignedDistanceField);

  private:
    glm::vec3 _center;
    glm::vec3 _extent;

  public:
    JarBoxSdf() : _center(0.0f, 0.0f, 0.0f), _extent(1.0f, 1.0f, 1.0f)
    {
    }
    void set_center(const Vector3 &center)
    {
        _center = glm::vec3(center.x, center.y, center.z);
    }
    Vector3 get_center() const
    {
        return Vector3(_center.x, _center.y, _center.z);
    }
    void set_extent(const Vector3 &extent)
    {
        _extent = glm::vec3(extent.x, extent.y, extent.z);
    }
    Vector3 get_extent() const
    {
        return Vector3(_extent.x, _extent.y, _extent.z);
    }

    virtual float distance(const glm::vec3 &pos) const override
    {
        glm::vec3 q = glm::abs(pos - _center) - _extent;
        return glm::length(glm::max(q, 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f);
    }

    virtual glm::vec3 normal(const glm::vec3 &pos) const override
    {
        glm::vec3 p = pos - _center;
        glm::vec3 q = glm::abs(p) - _extent;

        if (q.x > q.y && q.x > q.z) return glm::vec3((p.x > 0) ? 1 : -1, 0, 0);
        if (q.y > q.z)              return glm::vec3(0, (p.y > 0) ? 1 : -1, 0);
        return glm::vec3(0, 0, (p.z > 0) ? 1 : -1);
    }

  protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_center", "center"), &JarBoxSdf::set_center);
        ClassDB::bind_method(D_METHOD("get_center"), &JarBoxSdf::get_center);
        ClassDB::bind_method(D_METHOD("set_extent", "extent"), &JarBoxSdf::set_extent);
        ClassDB::bind_method(D_METHOD("get_extent"), &JarBoxSdf::get_extent);
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "center"), "set_center", "get_center");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "extent"), "set_extent", "get_extent");
    }

    virtual Bounds bounds() const override
    {
        return Bounds(_center - _extent, _center + _extent);
    }
};
}

#endif // AABB_SDF_H
