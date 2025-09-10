#pragma once
#include "signed_distance_field.h"
#include "bvh.h"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

namespace godot {

class JarMeshSdf : public JarSignedDistanceField {
    GDCLASS(JarMeshSdf, JarSignedDistanceField);

  public:
    JarMeshSdf() : _dirty(true) {}

    void set_mesh(const Ref<Mesh> &m) {
        _mesh = m;
        _dirty = true;
    }
    Ref<Mesh> get_mesh() const { return _mesh; }

    void set_transform(const Transform3D &xf) {
        _transform = xf;
        _dirty = true;
    }
    Transform3D get_transform() const { return _transform; }

    virtual float distance(const glm::vec3 &pos) const override {
        ensure_built();
        float sd; glm::vec3 n;
        _bvh.signed_distance_and_normal(pos, sd, n);
        return sd;
    }

    virtual glm::vec3 normal(const glm::vec3 &pos) const override {
        ensure_built();
        float sd; glm::vec3 n;
        _bvh.signed_distance_and_normal(pos, sd, n);
        return n;
    }

    virtual inline void distance_and_normal(const glm::vec3 &pos, float &out_distance, glm::vec3 &out_normal) const override {
        ensure_built();
        _bvh.signed_distance_and_normal(pos, out_distance, out_normal);
    }

    virtual Bounds bounds() const override {
        // Conservative: if built, expose BVH root bounds; otherwise empty
        if (_dirty || _bvh.nodes.empty()) {
            return Bounds(glm::vec3(0.0f), glm::vec3(0.0f));
        }
        const auto &b = _bvh.nodes[_bvh.root].bounds;
        return Bounds(b.min_corner, b.max_corner);
    }

  protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &JarMeshSdf::set_mesh);
        ClassDB::bind_method(D_METHOD("get_mesh"), &JarMeshSdf::get_mesh);
        ClassDB::bind_method(D_METHOD("set_transform", "transform"), &JarMeshSdf::set_transform);
        ClassDB::bind_method(D_METHOD("get_transform"), &JarMeshSdf::get_transform);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");
        ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "transform"), "set_transform", "get_transform");
    }

  private:
    mutable bool _dirty = true;
    Ref<Mesh> _mesh;
    Transform3D _transform;
    mutable sdf_bvh::BVH _bvh;

    void ensure_built() const {
        if (!_dirty) return;
        _dirty = false;

        Ref<ArrayMesh> arr_mesh = _mesh.is_valid() ? Ref<ArrayMesh>(Object::cast_to<ArrayMesh>(*_mesh)) : Ref<ArrayMesh>();
        Ref<ArrayMesh> final_mesh = arr_mesh;

        if (final_mesh.is_null() && _mesh.is_valid()) {
            // Fallback: SurfaceTool merge to ArrayMesh
            Ref<SurfaceTool> st;
            st.instantiate();
            for (int i = 0; i < _mesh->get_surface_count(); ++i) {
                st->create_from(_mesh, i);
            }
            final_mesh = st->commit();
        }

        _bvh.build_from_array_mesh(final_mesh, _transform);
    }
};

} // namespace godot
