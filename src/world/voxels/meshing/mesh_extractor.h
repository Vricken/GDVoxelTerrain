#ifndef MESH_EXTRACTOR_H
#define MESH_EXTRACTOR_H

#include "extracted_mesh_data.h"

namespace godot {

class JarVoxelTerrain;
class VoxelOctreeNode;

class MeshExtractor {
public:
    virtual ~MeshExtractor() = default;

    // Generate mesh data for a given chunk of terrain
    virtual ExtractedMeshData *generate_mesh_data(const JarVoxelTerrain &terrain,
                                                  const VoxelOctreeNode &chunk) = 0;
};

} // namespace godot

#endif // MESH_EXTRACTOR_H
