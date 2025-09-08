#ifndef STITCHED_SURFACE_NETS_H
#define STITCHED_SURFACE_NETS_H

#include "stitched_mesh_extractor.h"

class StitchedSurfaceNets : public StitchedMeshExtractor {
public:
    StitchedSurfaceNets(const JarVoxelTerrain& terrain);

    ChunkMeshData* generate_mesh_data(const JarVoxelTerrain& terrain, const VoxelOctreeNode& chunk) override;

protected:
    void create_vertex(const int node_id, const std::vector<int>& neighbours, bool on_ring) override;
};

#endif // STITCHED_SURFACE_NETS_H
