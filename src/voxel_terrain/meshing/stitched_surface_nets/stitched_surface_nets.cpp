#include "stitched_surface_nets.h"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "utility/utils.h"
#include "voxel_terrain.h"

StitchedSurfaceNets::StitchedSurfaceNets(const JarVoxelTerrain &terrain) : StitchedMeshExtractor(terrain)
{
    _cubicVoxels = (terrain.get_cubic_voxels());
}

void StitchedSurfaceNets::create_vertex(const int node_id, const std::vector<int> &neighbours, const bool on_ring)
{
    glm::vec3 vertexPosition(0.0f);
    glm::vec4 color(0, 0, 0, 0);
    glm::vec3 normal(0.0f);
    int duplicates = 0, edge_crossings = 0;
    for (auto &edge : StitchedMeshChunk::Edges)
    {
        auto ai = neighbours[edge.x];
        auto bi = neighbours[edge.y];
        auto na = _meshChunk->nodes[ai];
        auto nb = _meshChunk->nodes[bi];
        float valueA = na->get_value();
        float valueB = nb->get_value();
        glm::vec3 posA = na->_center;
        glm::vec3 posB = nb->_center;

        normal += (valueB - valueA) * (posB - posA);

        if (glm::sign(valueA) == glm::sign(valueB))
            continue;

        float t = glm::abs(valueA) / (glm::abs(valueA) + glm::abs(valueB));
        vertexPosition += glm::mix(posA, posB, t);
        edge_crossings++;
        color += glm::mix(na->get_color(), nb->get_color(), t);
    }

    if (edge_crossings <= 0)
        return;

    // computes and stores the directions in which to generate quads, also determines winding order
    _meshChunk->faceDirs[node_id] =
        (static_cast<int>(glm::sign(glm::sign(_meshChunk->nodes[neighbours[6]]->get_value()) -
                                    glm::sign(_meshChunk->nodes[neighbours[7]]->get_value())) +
                          1))
            << 0 |
        (static_cast<int>(glm::sign(glm::sign(_meshChunk->nodes[neighbours[7]]->get_value()) -
                                    glm::sign(_meshChunk->nodes[neighbours[5]]->get_value())) +
                          1))
            << 2 |
        (static_cast<int>(glm::sign(glm::sign(_meshChunk->nodes[neighbours[3]]->get_value()) -
                                    glm::sign(_meshChunk->nodes[neighbours[7]]->get_value())) +
                          1))
            << 4;

    vertexPosition /= static_cast<float>(edge_crossings);
    color /= static_cast<float>(edge_crossings);
    normal = glm::normalize(normal);
    if (_cubicVoxels)
        vertexPosition = _meshChunk->nodes[node_id]->_center;

    vertexPosition -= _chunk->_center;
    int vertexIndex = _verts.size();
    glm::ivec3 grid_position = _meshChunk->positions[node_id];
    if ((on_ring || _meshChunk->is_on_any_boundary(grid_position)) &&
        _meshChunk->should_have_boundary_quad(neighbours, on_ring))
    {
        if (on_ring)
            _ringEdgeNodes[grid_position] = node_id;
        else
            _innerEdgeNodes[grid_position] = node_id;
    }

    _meshChunk->vertexIndices[node_id] = vertexIndex;
    _verts.push_back({vertexPosition.x, vertexPosition.y, vertexPosition.z});
    _normals.push_back({normal.x, normal.y, normal.z});
    _colors.push_back({color.r, color.g, color.b, color.a});
}

ChunkMeshData* StitchedSurfaceNets::generate_mesh_data(const JarVoxelTerrain &terrain,
                                                                       const VoxelOctreeNode &chunk)
{
    _meshChunk = new StitchedMeshChunk(terrain, chunk); // TODO: make sure we overwrite existing memory
    _chunk = const_cast<VoxelOctreeNode *>(&chunk);
    
    for (size_t node_id = 0; node_id < _meshChunk->innerNodeCount; node_id++)
    {
        if (_meshChunk->vertexIndices[node_id] <= -2)
            continue;
        auto neighbours = std::vector<int>();
        glm::ivec3 grid_position = _meshChunk->positions[node_id];

        if (!_meshChunk->get_neighbours(grid_position, neighbours))
            continue;
        create_vertex(node_id, neighbours, false);
    }

    if (_verts.size() == 0)
        return nullptr;

    // if on lod boundary, add an additional pass
    for (size_t node_id = _meshChunk->innerNodeCount; node_id < _meshChunk->innerNodeCount + _meshChunk->ringNodeCount;
         node_id++)
    {
        if (_meshChunk->vertexIndices[node_id] <= -2)
            continue;
        auto neighbours = std::vector<int>();
        glm::ivec3 grid_position = _meshChunk->positions[node_id];

        if (!_meshChunk->get_ring_neighbours(grid_position, neighbours))
            continue;
        create_vertex(node_id, neighbours, true);
        // if (false)
        // { // print the nodes itself
        //     glm::vec3 vertexPosition = _meshChunk->nodes[node_id]->_center;
        //     vertexPosition -= _chunk->_center;
        //     int vertexIndex = (_verts.size());
        //     _meshChunk->vertexIndices[node_id] = vertexIndex;
        //     _verts.push_back({vertexPosition.x, vertexPosition.y, vertexPosition.z});
        //     _normals.push_back({0, 1, 0});
        //     _colors.push_back(Color(0, 0, 0, 0));
        //     _edgeIndices[grid_position] = (vertexIndex);
        // }
    }
    // godot::String ringNodes = "";
    // for (auto& [position, vertexId]: _ringEdgeNodes)
    // {
    //     ringNodes += Utils::to_string(position) + ", ";
    // }
    // godot::String innerNodes = "";
    // for (auto& [position, vertexId]: _innerEdgeNodes)
    // {
    //     innerNodes += Utils::to_string(position) + ", ";
    // }
    // UtilityFunctions::print("Ring Nodes: " + ringNodes);
    // UtilityFunctions::print("Inner Nodes: " + innerNodes);

    stitch_inner_faces();
    if (_indices.is_empty()) return nullptr;
    stitch_edge_chunk_boundaries();
    return build_chunk_mesh_data(terrain);
}