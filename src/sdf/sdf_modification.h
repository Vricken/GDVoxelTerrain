#ifndef SDF_MODIFICATION_H
#define SDF_MODIFICATION_H

#include "bounds.h"
#include "sdf_operations.h"
#include "signed_distance_field.h"
#include <glm/glm.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
namespace godot
{
struct ModifySettings
{
    Ref<JarSignedDistanceField> sdf;
    Bounds bounds;
    glm::vec3 position;
    SDFOperation operation;
    int material_index = 1;
    float smooth_k = 1.0f;

    glm::vec3 to_local(const glm::vec3 &point) const
    {
        return point - position;
    }
};

class JarSdfModification : public Resource
{
    GDCLASS(JarSdfModification, Resource);

  private:
    Ref<JarSignedDistanceField> _sdf;
    SDFOperation _operation = SDF_OPERATION_UNION;
    float _smoothK = 1.0f;
    float _max_radius = 1000.0f; // it may be less than this value if the sdf bounds are smaller. The modification will attempt to affect the smallest volume possible
    glm::vec3 _center{0.0f};

  public:
    void set_sdf(const Ref<JarSignedDistanceField> &sdf)
    {
        _sdf = sdf;
    }
    Ref<JarSignedDistanceField> get_sdf() const
    {
        return _sdf;
    }

    void set_operation(SDFOperation op)
    {
        _operation = op;
    }
    SDFOperation get_operation() const
    {
        return _operation;
    }

    void set_smooth_k(float k)
    {
        _smoothK = k;
    }
    float get_smooth_k() const
    {
        return _smoothK;
    }

    void set_max_radius(float r)
    {
        _max_radius = r;
    }
    float get_max_radius() const
    {
        return _max_radius;
    }

    void set_center(const Vector3 &c)
    {
        _center = glm::vec3(c.x, c.y, c.z);
    }
    Vector3 get_center() const
    {
        return Vector3(_center.x, _center.y, _center.z);
    }

    void set_center_glm(const glm::vec3 &c)
    {
        _center = c;
    }
    const glm::vec3 &get_center_glm() const
    {
        return _center;
    }

    ModifySettings to_settings(glm::vec3 octree_center, float bounds_buffer) const
    {
        ModifySettings s;
        s.sdf = _sdf;
        s.position = _center - octree_center;
        s.operation = _operation;
        s.smooth_k = _smoothK;

        // Get the bounds of the SDF, but make sure it does not exceed the specified radius
        Bounds sdfBounds = s.sdf->bounds().expanded(bounds_buffer).recentered(s.position);

        glm::vec3 halfExtents(_max_radius + bounds_buffer);
        Bounds radiusBounds(s.position - halfExtents, s.position + halfExtents);

        s.bounds = sdfBounds.intersected(radiusBounds);
        return s;
    }

  protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_sdf", "sdf"), &JarSdfModification::set_sdf);
        ClassDB::bind_method(D_METHOD("get_sdf"), &JarSdfModification::get_sdf);

        ClassDB::bind_method(D_METHOD("set_operation", "operation"), &JarSdfModification::set_operation);
        ClassDB::bind_method(D_METHOD("get_operation"), &JarSdfModification::get_operation);

        ClassDB::bind_method(D_METHOD("set_smooth_k", "k"), &JarSdfModification::set_smooth_k);
        ClassDB::bind_method(D_METHOD("get_smooth_k"), &JarSdfModification::get_smooth_k);

        ClassDB::bind_method(D_METHOD("set_max_radius", "max_radius"), &JarSdfModification::set_max_radius);
        ClassDB::bind_method(D_METHOD("get_max_radius"), &JarSdfModification::get_max_radius);

        ClassDB::bind_method(D_METHOD("set_center", "center"), &JarSdfModification::set_center);
        ClassDB::bind_method(D_METHOD("get_center"), &JarSdfModification::get_center);

        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sdf", PROPERTY_HINT_RESOURCE_TYPE, "JarSignedDistanceField"),
                     "set_sdf", "get_sdf");

        BIND_ENUM_CONSTANT(SDF_OPERATION_UNION);
        BIND_ENUM_CONSTANT(SDF_OPERATION_SUBTRACTION);
        BIND_ENUM_CONSTANT(SDF_OPERATION_INTERSECTION);
        BIND_ENUM_CONSTANT(SDF_OPERATION_SMOOTH_UNION);
        BIND_ENUM_CONSTANT(SDF_OPERATION_SMOOTH_SUBTRACTION);
        BIND_ENUM_CONSTANT(SDF_OPERATION_SMOOTH_INTERSECTION);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "operation", PROPERTY_HINT_ENUM,
                                  "Union,Subtraction,Intersection,SmoothUnion,SmoothSubtraction,SmoothIntersection"),
                     "set_operation", "get_operation");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "smooth_k"), "set_smooth_k", "get_smooth_k");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_radius"), "set_max_radius", "get_max_radius");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "center"), "set_center", "get_center");
    }

};
}

#endif // SDF_MODIFICATION_H
