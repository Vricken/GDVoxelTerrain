#include "stitched_dual_contouring.h"

#include <algorithm>
#include <glm/glm.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <limits>
#include <cmath>

#include "stitched_mesh_chunk.h"
#include "voxel_octree_node.h"
#include "voxel_terrain.h"
#include "dc_math.h"

using namespace godot;

inline bool is_finite3(const glm::vec3 &v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

StitchedDualContouring::StitchedDualContouring(const JarVoxelTerrain &terrain)
    : StitchedMeshExtractor(terrain) {
    _cubicVoxels = terrain.get_cubic_voxels();
}

void StitchedDualContouring::create_vertex(const int node_id, const std::vector<int> &neighbours, const bool on_ring) {
    // Gather Hermite samples from zero-crossing edges
    std::vector<HermiteSample> samples;
    samples.reserve(12);

    // For clamping, collect the 8 neighbour centers in this cell
    std::vector<glm::vec3> cell_pts;
    cell_pts.reserve(8);
    for (int i = 0; i < 8; ++i) {
        auto n = _meshChunk->nodes[neighbours[i]];
        cell_pts.push_back(n->_center);
    }

    for (auto &edge : StitchedMeshChunk::Edges) {
        int ai = neighbours[edge.x];
        int bi = neighbours[edge.y];

        auto na = _meshChunk->nodes[ai];
        auto nb = _meshChunk->nodes[bi];

        float va = na->get_value();
        float vb = nb->get_value();

        float t;
        if (!edge_zero_cross(va, vb, t))
            continue;

        // Ensure t is linear interpolation fraction
        t = glm::clamp(va / (va - vb), 0.0f, 1.0f);

        // Blend normals and orient consistently toward increasing SDF
        glm::vec3 n_mix = glm::normalize(glm::mix(na->get_normal(), nb->get_normal(), t));
        float vc = va * (1.0f - t) + vb * t;
        if (vc < 0.0f) n_mix = -n_mix;

        HermiteSample hs = interpolate_sample(
            na->_center, nb->_center,
            n_mix, n_mix,
            na->get_color(), nb->get_color(), t
        );

        samples.push_back(hs);
    }

    if (samples.empty())
        return;

    // Face directions from neighbour signs
    _meshChunk->faceDirs[node_id] = pack_face_dirs(neighbours, _meshChunk);

    QEF qef;
    glm::vec3 centroid(0.0f);
    glm::vec4 color_acc(0.0f);
    float color_w = 0.0f;

    for (const auto &s : samples) {
        qef.add(s.p, s.n, s.w);
        centroid += s.p;
        color_acc += s.c;
        color_w += 1.0f;
    }
    centroid *= (1.0f / float(samples.size()));
    glm::vec4 avg_color = (color_w > 0.0f) ? (color_acc * (1.0f / color_w)) : glm::vec4(0.0f);

    // Solve with scaled regularization
    glm::vec3 x;
    const float lambda = 1e-3f * _meshChunk->cell_size() * _meshChunk->cell_size();
    bool ok = (samples.size() >= 3) && qef.solve(x, centroid, lambda);
    if (!ok || !is_finite3(x))
        x = centroid;

    // Optional displacement guard
    glm::vec3 d = x - centroid;
    float max_disp = 0.5f * _meshChunk->cell_size();
    float d2 = glm::dot(d, d);
    if (d2 > max_disp * max_disp)
        x = centroid + d * (max_disp / glm::sqrt(d2));

    // Clamp to cell AABB
    x = clamp_to_cell(cell_pts, x);

    // Weighted blend of Hermite normals near solved x
    glm::vec3 nsum(0.0f);
    for (const auto &s : samples) {
        float dist = std::abs(glm::dot(s.n, x - s.p));
        float w = 1.0f / (1e-2f + dist);
        w = glm::min(w, 10.0f);
        nsum += s.n * w;
    }
    glm::vec3 nrm = glm::normalize(nsum);

    // Optional: in cubic mode, snap to the cell center
    if (_cubicVoxels) {
        x = _meshChunk->nodes[node_id]->_center;
    }

    glm::vec3 local = x - _chunk->_center;

    // Edge stitching bookkeeping
    glm::ivec3 grid_position = _meshChunk->positions[node_id];
    if ((on_ring || _meshChunk->is_on_any_boundary(grid_position)) &&
        _meshChunk->should_have_boundary_quad(neighbours, on_ring)) {
        if (on_ring)
            _ringEdgeNodes[grid_position] = node_id;
        else
            _innerEdgeNodes[grid_position] = node_id;
    }

    // Emit vertex
    int vertexIndex = static_cast<int>(_verts.size());
    _meshChunk->vertexIndices[node_id] = vertexIndex;
    _verts.push_back({local.x, local.y, local.z});
    _normals.push_back({nrm.x, nrm.y, nrm.z});
    _colors.push_back({avg_color.r, avg_color.g, avg_color.b, avg_color.a});
}

ChunkMeshData *StitchedDualContouring::generate_mesh_data(const JarVoxelTerrain &terrain, const VoxelOctreeNode &chunk) {
    _meshChunk = new StitchedMeshChunk(terrain, chunk);
    _chunk = const_cast<VoxelOctreeNode *>(&chunk);

    // Inner pass
    for (size_t node_id = 0; node_id < _meshChunk->innerNodeCount; ++node_id) {
        if (_meshChunk->vertexIndices[node_id] <= -2)
            continue;
        std::vector<int> neighbours;
        if (!_meshChunk->get_neighbours(_meshChunk->positions[node_id], neighbours))
            continue;
        create_vertex(static_cast<int>(node_id), neighbours, false);
    }

    if (_verts.is_empty())
        return nullptr;

    // Ring pass
    for (size_t node_id = _meshChunk->innerNodeCount;
         node_id < _meshChunk->innerNodeCount + _meshChunk->ringNodeCount;
         ++node_id) {
        if (_meshChunk->vertexIndices[node_id] <= -2)
            continue;
        std::vector<int> neighbours;
        if (!_meshChunk->get_ring_neighbours(_meshChunk->positions[node_id], neighbours))
            continue;
        create_vertex(static_cast<int>(node_id), neighbours, true);
    }

    stitch_inner_faces();
    if (_indices.is_empty())
        return nullptr;
    stitch_edge_chunk_boundaries();
    return build_chunk_mesh_data(terrain);
}
