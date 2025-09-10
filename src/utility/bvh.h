#pragma once
#include <vector>
#include <cstdint>
#include <limits>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/common.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include "bounds.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>


namespace sdf_bvh {

// -------------------------------------------
// Math helpers
// -------------------------------------------

struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec3 n0, n1, n2;      // optional; if zero-length, fallback to face normal
    godot::Bounds bbox;
    glm::vec3 face_n;

    void finalize() {
        bbox = godot::Bounds().joined(v0).joined(v1).joined(v2);
        face_n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        // zero normals default to face normal
        if (glm::length2(n0) < 1e-20f) n0 = face_n;
        if (glm::length2(n1) < 1e-20f) n1 = face_n;
        if (glm::length2(n2) < 1e-20f) n2 = face_n;
    }
};

struct BVHNode {
    godot::Bounds bounds;
    uint32_t left = 0;    // child index or start
    uint32_t right = 0;   // child index or count
    uint32_t is_leaf = 0; // 1 = leaf, 0 = inner
};

struct NearestHit {
    float sqr_dist = std::numeric_limits<float>::infinity();
    glm::vec3 closest = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    int tri_index = -1;
};

// -------------------------------------------
// BVH container
// -------------------------------------------

class BVH {
public:
    // CPU data
    std::vector<Triangle> triangles;
    std::vector<BVHNode> nodes;
    uint32_t root = 0;

    // Build from a Godot ArrayMesh (optionally with a transform to world)
    void build_from_array_mesh(const godot::Ref<godot::ArrayMesh> &mesh, const godot::Transform3D &xf);

    // Queries
    void nearest_point_and_normal(const glm::vec3 &p, NearestHit &out_hit) const;
    bool point_inside_solid(const glm::vec3 &p) const;

    // Combined signed distance + normal
    void signed_distance_and_normal(const glm::vec3 &p, float &out_sd, glm::vec3 &out_normal) const;

private:
    // Build internals
    struct PrimRef {
        uint32_t index;
        godot::Bounds bounds;
        glm::vec3 centroid;
    };
    std::vector<PrimRef> prims;

    uint32_t build_recursive(uint32_t start, uint32_t count);
    static godot::Bounds merge_range(const std::vector<PrimRef> &ps, uint32_t s, uint32_t c);
    static int choose_split_axis(const godot::Bounds &b);
    static uint32_t partition_prims(std::vector<PrimRef> &ps, uint32_t start, uint32_t count, int axis, float pivot);

    // Query internals
    static float aabb_point_sqr_distance(const godot::Bounds &b, const glm::vec3 &p);
    static bool ray_aabb_intersect(const godot::Bounds &b, const glm::vec3 &ro, const glm::vec3 &rd, float &tmin, float &tmax);

    static float point_triangle_query(const glm::vec3 &p, const Triangle &tri, glm::vec3 &closest, glm::vec3 &normal);

    // Ray-tri for inside test (Möller–Trumbore)
    static bool ray_triangle_intersect(const glm::vec3 &ro, const glm::vec3 &rd,
                                       const Triangle &tri, float &t, float &u, float &v);
};

} // namespace sdf_bvh
