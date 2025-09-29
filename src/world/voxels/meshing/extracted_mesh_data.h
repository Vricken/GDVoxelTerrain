#ifndef JAR_CHUNK_MESH_DATA_H
#define JAR_CHUNK_MESH_DATA_H

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <vector>
#include <unordered_map>
#include "bounds.h"
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace godot {

class ExtractedMeshData
{
  public:
    std::vector<Array> mesh_arrays;
    std::vector<int> material_indices;
    int lod;
    uint16_t boundaries;
    bool edge_chunk;
    Bounds bounds;
    std::unordered_map<glm::ivec3, int> edgeVertices;

    ExtractedMeshData(const std::vector<Array> &mesh_arrays, const std::vector<int> &material_indices, int lod, bool edge_chunk, const Bounds &chunk_bounds)
        : mesh_arrays(mesh_arrays), material_indices(material_indices), lod(lod), edge_chunk(edge_chunk), bounds(chunk_bounds)
    {
    }

    PackedVector3Array create_collision_mesh() const {
        PackedVector3Array collision_mesh;
        for (size_t i = 0; i < mesh_arrays.size(); i++) {
            Array surface_arrays = mesh_arrays[i];
            if (surface_arrays.size() == 0) {
                continue;
            }

            PackedVector3Array verts = surface_arrays[Mesh::ARRAY_VERTEX];
            PackedInt32Array indices = surface_arrays[Mesh::ARRAY_INDEX];

            for (size_t j = 0; j < indices.size(); j++) {
                collision_mesh.push_back(verts[indices[j]]);
            }
        }

        return collision_mesh;
    }
};
}

#endif // JAR_CHUNK_MESH_DATA_H
