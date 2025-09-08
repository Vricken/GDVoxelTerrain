#ifndef OPERATION_SDF_H
#define OPERATION_SDF_H

#include "signed_distance_field.h"
#include "sdf_operations.h"

class JarOperationSdf : public JarSignedDistanceField
{
    GDCLASS(JarOperationSdf, JarSignedDistanceField);

private:
    Ref<JarSignedDistanceField> _a;
    Ref<JarSignedDistanceField> _b;
    SDFOperation _operation = SDF_OPERATION_UNION;
    float _smoothK = 1.0f;

public:
    void set_sdf_a(const Ref<JarSignedDistanceField> &sdf) { _a = sdf; }
    Ref<JarSignedDistanceField> get_sdf_a() const { return _a; }

    void set_sdf_b(const Ref<JarSignedDistanceField> &sdf) { _b = sdf; }
    Ref<JarSignedDistanceField> get_sdf_b() const { return _b; }

    void set_operation(SDFOperation op) { _operation = op; }
    SDFOperation get_operation() const { return _operation; }

    void set_smooth_k(float k) { _smoothK = k; }
    float get_smooth_k() const { return _smoothK; }

    virtual float distance(const glm::vec3 &pos) const override
    {
        if (!_a.is_valid() || !_b.is_valid())
            return std::numeric_limits<float>::infinity();

        float da = _a->distance(pos);
        float db = _b->distance(pos);
        return SDF::apply_operation(_operation, da, db, _smoothK);
    }

    virtual glm::vec3 normal(const glm::vec3 &pos) const override
    {
        if (!_a.is_valid() || !_b.is_valid())
            return glm::vec3(0.0f, 1.0f, 0.0f);

        float da = _a->distance(pos);
        float db = _b->distance(pos);
        glm::vec3 na = _a->normal(pos);
        glm::vec3 nb = _b->normal(pos);

        float outValue;
        glm::vec3 outNormal;
        SDF::apply_operation(_operation, da, na, db, nb, _smoothK, outValue, outNormal);
        return outNormal;
    }

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_sdf_a", "sdf"), &JarOperationSdf::set_sdf_a);
        ClassDB::bind_method(D_METHOD("get_sdf_a"), &JarOperationSdf::get_sdf_a);
        ClassDB::bind_method(D_METHOD("set_sdf_b", "sdf"), &JarOperationSdf::set_sdf_b);
        ClassDB::bind_method(D_METHOD("get_sdf_b"), &JarOperationSdf::get_sdf_b);
        ClassDB::bind_method(D_METHOD("set_operation", "operation"), &JarOperationSdf::set_operation);
        ClassDB::bind_method(D_METHOD("get_operation"), &JarOperationSdf::get_operation);
        ClassDB::bind_method(D_METHOD("set_smooth_k", "k"), &JarOperationSdf::set_smooth_k);
        ClassDB::bind_method(D_METHOD("get_smooth_k"), &JarOperationSdf::get_smooth_k);

        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sdf_a", PROPERTY_HINT_RESOURCE_TYPE, "JarSignedDistanceField"), "set_sdf_a", "get_sdf_a");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sdf_b", PROPERTY_HINT_RESOURCE_TYPE, "JarSignedDistanceField"), "set_sdf_b", "get_sdf_b");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "operation", PROPERTY_HINT_ENUM, "Union,Subtraction,Intersection,SmoothUnion,SmoothSubtraction,SmoothIntersection"), "set_operation", "get_operation");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "smooth_k"), "set_smooth_k", "get_smooth_k");
    }
};

#endif // OPERATION_SDF_H
