#include "bvh.h"
#include <algorithm>
#include <stack>
#include <cmath>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

using namespace godot;
using namespace sdf_bvh;

// -------------------------------------------
// Utils
// -------------------------------------------

static inline glm::vec3 to_glm(const godot::Vector3 &v) { return glm::vec3(v.x, v.y, v.z); }
static inline glm::vec3 transform_point(const godot::Transform3D &xf, const glm::vec3 &p) {
    godot::Vector3 gp = xf.xform(godot::Vector3(p.x, p.y, p.z));
    return glm::vec3(gp.x, gp.y, gp.z);
}
static inline glm::vec3 transform_dir(const godot::Transform3D &xf, const glm::vec3 &d) {
    godot::Vector3 gd = xf.basis.xform(godot::Vector3(d.x, d.y, d.z));
    return glm::vec3(gd.x, gd.y, gd.z);
}

// -------------------------------------------
// Bounds / distance helpers
// -------------------------------------------

float BVH::aabb_point_sqr_distance(const Bounds &b, const glm::vec3 &p) {
    float s = 0.0f;
    for (int k = 0; k < 3; ++k) {
        float v = (&p.x)[k];
        float mn = (&b.min_corner.x)[k];
        float mx = (&b.max_corner.x)[k];
        float d = (v < mn) ? (mn - v) : ((v > mx) ? (v - mx) : 0.0f);
        s += d * d;
    }
    return s;
}

bool BVH::ray_aabb_intersect(const Bounds &b, const glm::vec3 &ro, const glm::vec3 &rd, float &tmin, float &tmax) {
    float lo = -std::numeric_limits<float>::infinity();
    float hi =  std::numeric_limits<float>::infinity();
    for (int i = 0; i < 3; ++i) {
        float inv = 1.0f / ((&rd.x)[i]);
        float t0 = ((&b.min_corner.x)[i] - (&ro.x)[i]) * inv;
        float t1 = ((&b.max_corner.x)[i] - (&ro.x)[i]) * inv;
        if (t0 > t1) std::swap(t0, t1);
        lo = std::max(lo, t0);
        hi = std::min(hi, t1);
        if (hi < lo) return false;
    }
    tmin = lo; tmax = hi; return true;
}

// -------------------------------------------
// Point-triangle distance with barycentrics
// Returns squared distance; outputs closest and smooth normal
// -------------------------------------------

float BVH::point_triangle_query(const glm::vec3 &p, const Triangle &tri, glm::vec3 &closest, glm::vec3 &normal) {
    // From "Real-Time Collision Detection" (Christer Ericson)
    const glm::vec3 &a = tri.v0, &b = tri.v1, &c = tri.v2;
    glm::vec3 ab = b - a, ac = c - a, ap = p - a;
    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) { closest = a; normal = tri.n0; return glm::length2(p - closest); }

    glm::vec3 bp = p - b;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) { closest = b; normal = tri.n1; return glm::length2(p - closest); }

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        closest = a + v * ab;
        normal = glm::normalize(glm::mix(tri.n0, tri.n1, v));
        return glm::length2(p - closest);
    }

    glm::vec3 cp = p - c;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) { closest = c; normal = tri.n2; return glm::length2(p - closest); }

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        closest = a + w * ac;
        normal = glm::normalize(glm::mix(tri.n0, tri.n2, w));
        return glm::length2(p - closest);
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        closest = b + w * (c - b);
        normal = glm::normalize(glm::mix(tri.n1, tri.n2, w));
        return glm::length2(p - closest);
    }

    // Inside face
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    float u = 1.0f - v - w;
    closest = u * a + v * b + w * c;
    normal = glm::normalize(u * tri.n0 + v * tri.n1 + w * tri.n2);
    return glm::length2(p - closest);
}

// -------------------------------------------
// Ray-triangle
// -------------------------------------------

bool BVH::ray_triangle_intersect(const glm::vec3 &ro, const glm::vec3 &rd,
                                 const Triangle &tri, float &t, float &u, float &v) {
    const float EPS = 1e-8f;
    glm::vec3 e1 = tri.v1 - tri.v0;
    glm::vec3 e2 = tri.v2 - tri.v0;
    glm::vec3 p = glm::cross(rd, e2);
    float det = glm::dot(e1, p);
    if (std::fabs(det) < EPS) return false;
    float invDet = 1.0f / det;
    glm::vec3 s = ro - tri.v0;
    u = glm::dot(s, p) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    glm::vec3 q = glm::cross(s, e1);
    v = glm::dot(rd, q) * invDet;
    if (v < 0.0f || (u + v) > 1.0f) return false;
    t = glm::dot(e2, q) * invDet;
    return t > EPS;
}

// -------------------------------------------
// Build
// -------------------------------------------

Bounds BVH::merge_range(const std::vector<PrimRef> &ps, uint32_t s, uint32_t c) {
    Bounds b{};
    for (uint32_t i = 0; i < c; ++i) b = b.joined(ps[s + i].bounds);
    return b;
}

int BVH::choose_split_axis(const Bounds &b) {
    glm::vec3 e = b.get_size();
    if (e.x > e.y && e.x > e.z) return 0;
    if (e.y > e.z) return 1;
    return 2;
}

uint32_t BVH::partition_prims(std::vector<PrimRef> &ps, uint32_t start, uint32_t count, int axis, float pivot) {
    uint32_t i = start, j = start + count - 1;
    while (i <= j) {
        if ((&ps[i].centroid.x)[axis] < pivot) { ++i; }
        else { std::swap(ps[i], ps[j]); if (j == 0) break; --j; }
    }
    return i;
}

uint32_t BVH::build_recursive(uint32_t start, uint32_t count) {
    BVHNode node;
    node.bounds = merge_range(prims, start, count);

    if (count <= 8) {
        node.is_leaf = 1;
        node.left = start;
        node.right = count;
        uint32_t idx = (uint32_t)nodes.size();
        nodes.push_back(node);
        return idx;
    }

    int axis = choose_split_axis(node.bounds);
    float pivot = ((&node.bounds.min_corner.x)[axis] + (&node.bounds.min_corner.x)[axis]) * 0.5f;
    uint32_t mid = partition_prims(prims, start, count, axis, pivot);
    if (mid == start || mid == start + count) {
        // Fallback: median
        mid = start + count / 2;
        std::nth_element(prims.begin() + start, prims.begin() + mid, prims.begin() + start + count,
            [axis](const PrimRef &a, const PrimRef &b) {
                return (&a.centroid.x)[axis] < (&b.centroid.x)[axis];
            });
    }

    uint32_t idx = (uint32_t)nodes.size();
    nodes.push_back(BVHNode{});
    uint32_t L = build_recursive(start, mid - start);
    uint32_t R = build_recursive(mid, start + count - mid);

    nodes[idx].bounds = node.bounds;
    nodes[idx].is_leaf = 0;
    nodes[idx].left = L;
    nodes[idx].right = R;
    return idx;
}

void BVH::build_from_array_mesh(const Ref<ArrayMesh> &mesh, const Transform3D &xf) {
    triangles.clear();
    nodes.clear();
    prims.clear();

    if (mesh.is_null()) return;

    // Gather triangles + (optional) vertex normals
    const int surf_count = mesh->get_surface_count();
    for (int si = 0; si < surf_count; ++si) {
        Array arr = mesh->surface_get_arrays(si);
        PackedVector3Array gd_pos = arr[Mesh::ARRAY_VERTEX];
        PackedVector3Array gd_nrm = arr[Mesh::ARRAY_NORMAL];

        PackedInt32Array gd_idx;
        if (arr.size() > Mesh::ARRAY_INDEX) {
            gd_idx = arr[Mesh::ARRAY_INDEX];
        }

        auto fetch_v = [&](int idx) -> glm::vec3 {
            return transform_point(xf, to_glm(gd_pos[idx]));
        };
        auto fetch_n = [&](int idx) -> glm::vec3 {
            glm::vec3 n = gd_nrm.size() > 0 ? to_glm(gd_nrm[idx]) : glm::vec3(0.0f);
            return glm::normalize(transform_dir(xf, n));
        };

        if (gd_idx.size() > 0) {
            for (int i = 0; i + 2 < gd_idx.size(); i += 3) {
                Triangle t{};
                int i0 = gd_idx[i + 0], i1 = gd_idx[i + 1], i2 = gd_idx[i + 2];
                t.v0 = fetch_v(i0); t.v1 = fetch_v(i1); t.v2 = fetch_v(i2);
                t.n0 = fetch_n(i0); t.n1 = fetch_n(i1); t.n2 = fetch_n(i2);
                t.finalize();
                triangles.push_back(t);
            }
        } else {
            for (int i = 0; i + 2 < gd_pos.size(); i += 3) {
                Triangle t{};
                t.v0 = fetch_v(i + 0); t.v1 = fetch_v(i + 1); t.v2 = fetch_v(i + 2);
                t.n0 = fetch_n(i + 0); t.n1 = fetch_n(i + 1); t.n2 = fetch_n(i + 2);
                t.finalize();
                triangles.push_back(t);
            }
        }
    }

    // Build prim refs
    prims.reserve(triangles.size());
    for (uint32_t i = 0; i < triangles.size(); ++i) {
        PrimRef pr{};
        pr.index = i;
        pr.bounds = triangles[i].bbox;
        pr.centroid = (triangles[i].v0 + triangles[i].v1 + triangles[i].v2) / 3.0f;
        prims.push_back(pr);
    }

    if (prims.empty()) {
        root = 0;
        return;
    }

    // Build hierarchy
    nodes.reserve(prims.size() * 2);
    root = build_recursive(0, (uint32_t)prims.size());

    // Reorder triangles to match leaf ranges for cache locality
    std::vector<Triangle> reordered(triangles.size());
    std::vector<uint32_t> map(prims.size());
    for (uint32_t i = 0; i < prims.size(); ++i) map[i] = prims[i].index;
    // Remap leaves
    for (BVHNode &n : nodes) {
        if (n.is_leaf) {
            for (uint32_t i = 0; i < n.right; ++i) {
                reordered[n.left + i] = triangles[map[n.left + i]];
            }
        }
    }
    triangles.swap(reordered);
}

// -------------------------------------------
// Nearest point query
// -------------------------------------------

void BVH::nearest_point_and_normal(const glm::vec3 &p, NearestHit &out_hit) const {
    if (nodes.empty()) return;

    struct StackItem { uint32_t idx; float sqd; };
    std::vector<StackItem> stack;
    stack.reserve(64);

    auto push = [&](uint32_t idx) {
        float d2 = aabb_point_sqr_distance(nodes[idx].bounds, p);
        stack.push_back({idx, d2});
    };

    push(root);

    while (!stack.empty()) {
        // Pop the item with smallest bound distance (linear find min)
        size_t best_i = 0;
        for (size_t i = 1; i < stack.size(); ++i)
            if (stack[i].sqd < stack[best_i].sqd) best_i = i;
        auto cur = stack[best_i];
        stack[best_i] = stack.back(); stack.pop_back();

        if (cur.sqd >= out_hit.sqr_dist) continue; // prune
        const BVHNode &n = nodes[cur.idx];

        if (n.is_leaf) {
            for (uint32_t i = 0; i < n.right; ++i) {
                const Triangle &tri = triangles[n.left + i];
                glm::vec3 cp, nn;
                float d2 = point_triangle_query(p, tri, cp, nn);
                if (d2 < out_hit.sqr_dist) {
                    out_hit.sqr_dist = d2;
                    out_hit.closest = cp;
                    out_hit.normal = nn;
                    out_hit.tri_index = int(n.left + i);
                }
            }
        } else {
            push(n.left);
            push(n.right);
        }
    }
}

// -------------------------------------------
// Inside test (ray parity along +X)
// -------------------------------------------

bool BVH::point_inside_solid(const glm::vec3 &p) const {
    if (nodes.empty()) return false;

    // Nudge to avoid grazing edges
    const float EPS = 1e-4f;
    glm::vec3 ro = p + glm::vec3(EPS * 3.0f, EPS * 5.0f, EPS * 7.0f);
    glm::vec3 rd = glm::vec3(1.0f, 0.0f, 0.0f);

    int hits = 0;
    std::vector<uint32_t> stack;
    stack.reserve(64);
    stack.push_back(root);

    while (!stack.empty()) {
        uint32_t idx = stack.back(); stack.pop_back();
        float tmin, tmax;
        if (!ray_aabb_intersect(nodes[idx].bounds, ro, rd, tmin, tmax)) continue;

        const BVHNode &n = nodes[idx];
        if (n.is_leaf) {
            for (uint32_t i = 0; i < n.right; ++i) {
                const Triangle &tri = triangles[n.left + i];
                float t, u, v;
                if (ray_triangle_intersect(ro, rd, tri, t, u, v)) {
                    if (t > 0.0f) ++hits;
                }
            }
        } else {
            stack.push_back(n.left);
            stack.push_back(n.right);
        }
    }

    return (hits & 1) == 1; // odd = inside
}

// -------------------------------------------
// Combined SD + normal
// -------------------------------------------

void BVH::signed_distance_and_normal(const glm::vec3 &p, float &out_sd, glm::vec3 &out_normal) const {
    NearestHit hit;
    nearest_point_and_normal(p, hit);

    if (hit.tri_index < 0) {
        out_sd = std::numeric_limits<float>::infinity();
        out_normal = glm::vec3(0.0f, 1.0f, 0.0f);
        return;
    }

    float d = std::sqrt(hit.sqr_dist);
    bool inside = point_inside_solid(p);

    out_sd = inside ? -d : d;
    // For SDF convention, normals point outward; when inside, keep normal outward
    out_normal = glm::normalize(hit.normal);
}
