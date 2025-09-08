#ifndef MESH_EXTRACTOR_H
#define MESH_EXTRACTOR_H

#include "chunk_mesh_data.h"


namespace godot {

class JarVoxelTerrain;
class VoxelOctreeNode;

// Base interface for any mesh extractor
class MeshExtractor {
public:
    virtual ~MeshExtractor() = default;

    virtual ChunkMeshData* generate_mesh_data(const JarVoxelTerrain &terrain, const VoxelOctreeNode &chunk) = 0;
};
}

#endif // MESH_EXTRACTOR_H
