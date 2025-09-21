#ifndef MESH_EXTRACTOR_H
#define MESH_EXTRACTOR_H

#include "chunk_mesh_data.h"

namespace godot
{

class JarVoxelTerrain;
class VoxelOctreeNode;

// Base interface for any mesh extractor
class MeshExtractor
{
  protected:
    PackedVector3Array _verts;
    PackedVector3Array _normals;
    PackedColorArray _colors;
    PackedInt32Array _indices;

  public:
    virtual ~MeshExtractor() = default;

    virtual ChunkMeshData *generate_mesh_data(const JarVoxelTerrain &terrain, const VoxelOctreeNode &chunk) = 0;

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
    virtual void create_vertex(const int node_id, const std::vector<int> &neighbours, bool on_ring) = 0;
};
} // namespace godot

#endif // MESH_EXTRACTOR_H
