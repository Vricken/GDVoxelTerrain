#ifndef STITCHED_SURFACE_NETS_H
#define STITCHED_SURFACE_NETS_H

#include "mesh_extractor.h"
#include "mesh_chunk.h"

namespace godot
{

class StitchedSurfaceNets : public MeshExtractor
{
  public:
    StitchedSurfaceNets(const JarVoxelTerrain &terrain);

    ChunkMeshData *generate_mesh_data(const JarVoxelTerrain &terrain, const VoxelOctreeNode &chunk) override;

  protected:
    void create_vertex(const int node_id, const std::vector<int> &neighbours, bool on_ring) override;
    std::unordered_map<glm::ivec3, int> _innerEdgeNodes;
    std::unordered_map<glm::ivec3, int> _ringEdgeNodes;

    bool _cubicVoxels;
    VoxelOctreeNode *_chunk;
    MeshChunk *_meshChunk;

    std::vector<std::vector<int>> find_ring_nodes(const glm::ivec3 &pos, const int face) const;

    void stitch_inner_faces();

    void stitch_edge_chunk_boundaries();

    ChunkMeshData *build_chunk_mesh_data(const JarVoxelTerrain &terrain) const;
};
} // namespace godot

#endif // STITCHED_SURFACE_NETS_H
