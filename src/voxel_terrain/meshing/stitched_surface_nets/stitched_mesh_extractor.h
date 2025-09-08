#ifndef STITCHED_MESH_EXTRACTOR_H
#define STITCHED_MESH_EXTRACTOR_H

#include "mesh_extractor.h"
#include "stitched_mesh_chunk.h"
#include "voxel_octree_node.h"
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <unordered_map>
#include <vector>

using namespace godot;

class StitchedMeshExtractor : public MeshExtractor
{
  protected:
    PackedVector3Array _verts;
    PackedVector3Array _normals;
    PackedColorArray _colors;
    PackedInt32Array _indices;
    // std::vector<bool> _badNormals;

    std::unordered_map<glm::ivec3, int> _innerEdgeNodes;
    std::unordered_map<glm::ivec3, int> _ringEdgeNodes;

    bool _cubicVoxels;
    VoxelOctreeNode *_chunk;
    StitchedMeshChunk *_meshChunk;

    StitchedMeshExtractor(const JarVoxelTerrain &terrain);

    // I'd rather not base the winding order on the normal, but it works for now. Only required for the edge chunk.
    inline void add_tri_fix_normal(int n0, int n1, int n2)
    {
        godot::Vector3 normal = (_verts[n1] - _verts[n0]).cross(_verts[n2] - _verts[n0]);
        add_tri(n0, n1, n2, normal.dot(_normals[n0]) > 0);
    }

    inline void add_tri(int n0, int n1, int n2, bool flip)
    {
        if (!flip)
        {
            _indices.push_back(n0);
            _indices.push_back(n1);
            _indices.push_back(n2);
        }
        else
        {
            _indices.push_back(n1);
            _indices.push_back(n0);
            _indices.push_back(n2);
        }
    }
    std::vector<std::vector<int>> find_ring_nodes(const glm::ivec3 &pos, const int face) const;

    virtual void create_vertex(const int node_id, const std::vector<int> &neighbours, bool on_ring) = 0;

    void stitch_inner_faces();


    void stitch_edge_chunk_boundaries();

    ChunkMeshData* build_chunk_mesh_data(const JarVoxelTerrain& terrain) const;


  public:
    virtual ~StitchedMeshExtractor() = default;
};

#endif // STITCHED_MESH_EXTRACTOR_H
