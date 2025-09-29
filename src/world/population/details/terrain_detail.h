#ifndef JAR_TERRAIN_DETAIL_H
#define JAR_TERRAIN_DETAIL_H

#include <algorithm>
#include <glm/glm.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <optional>
#include "terrain_populator.h"
#include <set>

namespace godot {

class JarTerrainDetail : public JarTerrainPopulator
{
    GDCLASS(JarTerrainDetail, JarTerrainPopulator);

  private:
    Ref<Mesh> mesh;
    Ref<Material> material;
    float density = 1.0f;
    int max_lod = 0;
    bool shadows_enabled = false;

    bool material_filtering_use_whitelist = true;
    PackedInt32Array material_filtering_material_ids = {0};
    std::set<int> material_filtering_material_id_set = {0};

    void rebuild_material_id_set() {
        material_filtering_material_id_set.clear();
        for (int i = 0; i < material_filtering_material_ids.size(); i++) {
            material_filtering_material_id_set.insert(material_filtering_material_ids[i]);
        }
    }

  public:
    virtual ~JarTerrainDetail() = default;

    Ref<Mesh> get_mesh() const { return mesh; }
    Ref<Material> get_material() const { return material; }
    float get_density() const { return density; }
    int get_max_lod() const { return max_lod; }
    bool get_shadows_enabled() const { return shadows_enabled; }

    void set_mesh(const Ref<Mesh> &value) { mesh = value; }
    void set_material(const Ref<Material> &value) { material = value; }
    void set_density(float value) { density = value; }
    void set_max_lod(int value) { max_lod = value; }
    void set_shadows_enabled(bool value) { shadows_enabled = value; }

    bool get_material_filtering_use_whitelist() const { return material_filtering_use_whitelist; }
    void set_material_filtering_use_whitelist(bool value) { material_filtering_use_whitelist = value; }

    PackedInt32Array get_material_filtering_material_ids() const { return material_filtering_material_ids; }
    void set_material_filtering_material_ids(const PackedInt32Array &ids) {
        material_filtering_material_ids = ids;
        rebuild_material_id_set();
    }

    bool supports_material(int mat_id) const {
        bool in_set = material_filtering_material_id_set.find(mat_id) != material_filtering_material_id_set.end();
        return material_filtering_use_whitelist ? in_set : !in_set;
    }

  protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("get_mesh"), &JarTerrainDetail::get_mesh);
        ClassDB::bind_method(D_METHOD("set_mesh", "value"), &JarTerrainDetail::set_mesh);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");

        ClassDB::bind_method(D_METHOD("get_material"), &JarTerrainDetail::get_material);
        ClassDB::bind_method(D_METHOD("set_material", "value"), &JarTerrainDetail::set_material);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material", "get_material");

        ClassDB::bind_method(D_METHOD("get_density"), &JarTerrainDetail::get_density);
        ClassDB::bind_method(D_METHOD("set_density", "value"), &JarTerrainDetail::set_density);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density"), "set_density", "get_density");

        ClassDB::bind_method(D_METHOD("get_max_lod"), &JarTerrainDetail::get_max_lod);
        ClassDB::bind_method(D_METHOD("set_max_lod", "value"), &JarTerrainDetail::set_max_lod);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "max_lod"), "set_max_lod", "get_max_lod");

        ClassDB::bind_method(D_METHOD("get_shadows_enabled"), &JarTerrainDetail::get_shadows_enabled);
        ClassDB::bind_method(D_METHOD("set_shadows_enabled", "value"), &JarTerrainDetail::set_shadows_enabled);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shadows_enabled"), "set_shadows_enabled", "get_shadows_enabled");

        ADD_GROUP("Material Filtering", "material_filtering_");

        ClassDB::bind_method(D_METHOD("get_material_filtering_use_whitelist"), &JarTerrainDetail::get_material_filtering_use_whitelist);
        ClassDB::bind_method(D_METHOD("set_material_filtering_use_whitelist", "value"), &JarTerrainDetail::set_material_filtering_use_whitelist);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "material_filtering_use_whitelist"), "set_material_filtering_use_whitelist", "get_material_filtering_use_whitelist");

        ClassDB::bind_method(D_METHOD("get_material_filtering_material_ids"), &JarTerrainDetail::get_material_filtering_material_ids);
        ClassDB::bind_method(D_METHOD("set_material_filtering_material_ids", "ids"), &JarTerrainDetail::set_material_filtering_material_ids);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "material_filtering_ids"), "set_material_filtering_material_ids", "get_material_filtering_material_ids");

        ClassDB::bind_method(D_METHOD("supports_material", "mat_id"), &JarTerrainDetail::supports_material);
    }
};

} // namespace godot

#endif // JAR_TERRAIN_DETAIL_H
