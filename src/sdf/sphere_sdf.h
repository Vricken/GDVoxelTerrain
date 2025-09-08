#ifndef SPHERE_SDF_H
#define SPHERE_SDF_H

#include "signed_distance_field.h"

namespace godot
{
class JarSphereSdf : public JarSignedDistanceField
{
    GDCLASS(JarSphereSdf, JarSignedDistanceField);

  private:
    float _radius;

  public:
    JarSphereSdf() : _radius(1.0f)
    {
    }

    void set_radius(float radius)
    {
        _radius = radius;
    }
    float get_radius() const
    {
        return _radius;
    }

    virtual float distance(const glm::vec3 &pos) const override
    {
        return glm::length(pos) - _radius;
    }

    virtual glm::vec3 normal(const glm::vec3 &pos) const override
    {
        return glm::normalize(pos);
    }

    virtual Bounds bounds() const override
    {
        return Bounds(-glm::vec3(_radius), glm::vec3(_radius));
    }

  protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_radius", "radius"), &JarSphereSdf::set_radius);
        ClassDB::bind_method(D_METHOD("get_radius"), &JarSphereSdf::get_radius);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius"), "set_radius", "get_radius");
    }
};
}

#endif // SPHERE_SDF_H
