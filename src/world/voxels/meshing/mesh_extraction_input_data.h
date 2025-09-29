#ifndef MESH_EXTRACTION_INPUT_DATA_H
#define MESH_EXTRACTION_INPUT_DATA_H

namespace godot {

class JarVoxelTerrain;
class VoxelOctreeNode;

class MeshExtractionInputData {
public:
    virtual ~MeshExtractionInputData() = default;
};

} // namespace godot

#endif // MESH_EXTRACTOR_H
