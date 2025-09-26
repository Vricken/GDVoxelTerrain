#ifndef CHUNK_DETAIL_GENERATOR_H
#define CHUNK_DETAIL_GENERATOR_H

#include <godot_cpp/classes/multi_mesh.hpp>
#include <vector>
// #include <godot_cpp/classes/array.hpp>
// #include <godot_cpp/classes/transform3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/templates/hashfuncs.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "terrain_detail.h"
#include "voxel_terrain_material.h"

namespace godot
{

class JarVoxelTerrain;
class ExtractedMeshData;
class JarWorld;

class ChunkDetailGenerator
{
  private:
    struct Triangle
    {
        int a, b, c;
        uint32_t material_index;
    };

    JarWorld *world;
    VoxelMaterialMode material_mode;

    float DeterministicFloat(const Vector3 &input) const
    {
        // Generate a deterministic float based on the input vector
        uint32_t hash = HashMapHasherDefault::hash(input);
        return static_cast<float>(hash % 10000) / 10000.0f; // Normalize to [0, 1]
    }

    Vector3 get_gravity_normal(const Vector3 &position) const;
    float get_height(const Vector3 &position) const;
    uint32_t get_material_index_from_triangle(const Triangle &tri, const std::vector<Color> &colors, const glm::vec2 &barycentric) const;

  public:
    ChunkDetailGenerator(JarWorld *world, VoxelMaterialMode material_mode);
    ~ChunkDetailGenerator();

    TypedArray<MultiMesh> generate_details(const TypedArray<JarTerrainDetail> &details,
                                           const ExtractedMeshData &chunkMeshData);
};
} // namespace godot

#endif // CHUNK_DETAIL_GENERATOR_H