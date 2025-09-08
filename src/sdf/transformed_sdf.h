#ifndef TRASNFORMED_SDF_H
#define TRASNFORMED_SDF_H

#include "signed_distance_field.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace godot
{
class JarTransformedSdf : public JarSignedDistanceField
{
    GDCLASS(JarTransformedSdf, JarSignedDistanceField);

private:
    Ref<JarSignedDistanceField> _source;
    glm::vec3 _translation{0.0f};
    glm::quat _rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 _scale{1.0f};

    // Cached matrices
    glm::mat4 _transform{1.0f};
    glm::mat4 _invTransform{1.0f};

    void _update_transform()
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), _translation);
        glm::mat4 R = glm::toMat4(_rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), _scale);
        _transform = T * R * S;
        _invTransform = glm::inverse(_transform);
    }

public:
    JarTransformedSdf()
    {
        _update_transform();
    }

    void set_source(const Ref<JarSignedDistanceField> &sdf) { _source = sdf; }
    Ref<JarSignedDistanceField> get_source() const { return _source; }

    void set_translation(const Vector3 &t) { _translation = glm::vec3(t.x, t.y, t.z); _update_transform(); }
    Vector3 get_translation() const { return Vector3(_translation.x, _translation.y, _translation.z); }

    void set_rotation_quat(const Quaternion &q) { _rotation = glm::quat(q.w, q.x, q.y, q.z); _update_transform(); }
    Quaternion get_rotation_quat() const { return Quaternion(_rotation.x, _rotation.y, _rotation.z, _rotation.w); }

    void set_scale(const Vector3 &s) { _scale = glm::vec3(s.x, s.y, s.z); _update_transform(); }
    Vector3 get_scale() const { return Vector3(_scale.x, _scale.y, _scale.z); }

    virtual float distance(const glm::vec3 &pos) const override
    {
        if (!_source.is_valid())
            return std::numeric_limits<float>::infinity();

        // Transform point into local space of the source SDF
        glm::vec3 localPos = glm::vec3(_invTransform * glm::vec4(pos, 1.0f));

        // If non-uniform scale, distance should be adjusted
        float scaleFactor = glm::compMin(glm::abs(_scale));
        return _source->distance(localPos) * scaleFactor;
    }

    virtual glm::vec3 normal(const glm::vec3 &pos) const override
    {
        if (!_source.is_valid())
            return glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 localPos = glm::vec3(_invTransform * glm::vec4(pos, 1.0f));
        glm::vec3 localNormal = _source->normal(localPos);

        // Transform normal back to world space (ignore translation)
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(_transform)));
        return glm::normalize(normalMatrix * localNormal);
    }

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_source", "sdf"), &JarTransformedSdf::set_source);
        ClassDB::bind_method(D_METHOD("get_source"), &JarTransformedSdf::get_source);
        ClassDB::bind_method(D_METHOD("set_translation", "translation"), &JarTransformedSdf::set_translation);
        ClassDB::bind_method(D_METHOD("get_translation"), &JarTransformedSdf::get_translation);
        ClassDB::bind_method(D_METHOD("set_rotation_quat", "rotation"), &JarTransformedSdf::set_rotation_quat);
        ClassDB::bind_method(D_METHOD("get_rotation_quat"), &JarTransformedSdf::get_rotation_quat);
        ClassDB::bind_method(D_METHOD("set_scale", "scale"), &JarTransformedSdf::set_scale);
        ClassDB::bind_method(D_METHOD("get_scale"), &JarTransformedSdf::get_scale);

        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source", PROPERTY_HINT_RESOURCE_TYPE, "JarSignedDistanceField"), "set_source", "get_source");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "translation"), "set_translation", "get_translation");
        ADD_PROPERTY(PropertyInfo(Variant::QUATERNION, "rotation"), "set_rotation_quat", "get_rotation_quat");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "scale"), "set_scale", "get_scale");
    }

    virtual Bounds bounds() const override
    {
        if (!_source.is_valid())
            return Bounds();

        // Transform all 8 corners of the source bounds
        Bounds srcBounds = _source->bounds();
        glm::vec3 corners[8] = {
            {srcBounds.min_corner.x, srcBounds.min_corner.y, srcBounds.min_corner.z},
            {srcBounds.max_corner.x, srcBounds.min_corner.y, srcBounds.min_corner.z},
            {srcBounds.min_corner.x, srcBounds.max_corner.y, srcBounds.min_corner.z},
            {srcBounds.min_corner.x, srcBounds.min_corner.y, srcBounds.max_corner.z},
            {srcBounds.max_corner.x, srcBounds.max_corner.y, srcBounds.min_corner.z},
            {srcBounds.max_corner.x, srcBounds.min_corner.y, srcBounds.max_corner.z},
            {srcBounds.min_corner.x, srcBounds.max_corner.y, srcBounds.max_corner.z},
            {srcBounds.max_corner.x, srcBounds.max_corner.y, srcBounds.max_corner.z}
        };

        Bounds worldBounds;
        bool first = true;
        for (auto &c : corners)
        {
            glm::vec3 wc = glm::vec3(_transform * glm::vec4(c, 1.0f));
            if (first)
            {
                worldBounds.min_corner = wc;
                worldBounds.max_corner = wc;
                first = false;
            }
            else
            {
                worldBounds.min_corner = glm::min(worldBounds.min_corner, wc);
                worldBounds.max_corner = glm::max(worldBounds.max_corner, wc);
            }
        }
        return worldBounds;
    }
};
}

#endif // TRASNFORMED_SDF_H
