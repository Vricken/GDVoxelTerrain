#include "stitched_mesh_extractor.h"
#include "voxel_terrain/voxel_terrain.h"

StitchedMeshExtractor::StitchedMeshExtractor(const JarVoxelTerrain &terrain)
{
}

//this function finds way too many possible nodes.
//Possible improvement: check the octant, e.g. we dont need to find ringnodes towards neighbours of the same LOD
//Possible improvement: use the same system of a crossed edge as before to verify if we need a quad or not
std::vector<std::vector<int>> StitchedMeshExtractor::find_ring_nodes(const glm::ivec3 &pos, const int face) const
{
    static const glm::ivec3 face_offsets[3] = {glm::ivec3(1, 0, 0), glm::ivec3(0, 1, 0), glm::ivec3(0, 0, 1)};
    static const glm::ivec3 ring_offsets[3][8] = {
        {glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1), glm::ivec3(0, 1, 1), glm::ivec3(0, -1, -1), glm::ivec3(0, -1, 1), glm::ivec3(0, 1, -1)},
        {glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1), glm::ivec3(1, 0, 1), glm::ivec3(-1, 0, -1), glm::ivec3(-1, 0, 1), glm::ivec3(1, 0, -1)},
        {glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0), glm::ivec3(1, 1, 0), glm::ivec3(-1, -1, 0), glm::ivec3(-1, 1, 0), glm::ivec3(1, -1, 0)}};

        // static const glm::ivec3 ring_offsets[3][4] = {
        //     {glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)},
        //     {glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)},
        //     {glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0)}};
    // function to go from inner coordinates to ring coordinates
    auto get_ring_node = [this](const glm::ivec3 pos) {
        glm::ivec3 ring_pos = glm::floor((glm::vec3(pos)) / 2.0f);

        auto it = _ringEdgeNodes.find(ring_pos);
        if (it == _ringEdgeNodes.end() || it->second < 0 || _meshChunk->vertexIndices[it->second] < 0)
            return -1;

        return it->second;
    };

    std::vector<std::vector<int>> result;
    for (size_t i = 0; i < 3; i++)//check all directions?
    {
        for (auto dir : ring_offsets[i])
        {
            int n0 = get_ring_node(pos + dir);
            int n1 = get_ring_node(pos + dir + face_offsets[i]);
    
            if (n0 >= 0 && n1 >= 0)
                result.push_back({n0, n1});
            else if (n0 >= 0)
                result.push_back({n0});
            else if (n1 >= 0)
                result.push_back({n1});
        }
    }
    

    return result;
}

void StitchedMeshExtractor::stitch_inner_faces() {
    for (size_t node_id = 0; node_id < _meshChunk->innerNodeCount; ++node_id) {
        if (_meshChunk->vertexIndices[node_id] <= -1) continue;

        const auto pos = _meshChunk->positions[node_id];
        const int faceDirs = _meshChunk->faceDirs[node_id];

        static const int faces = 3;
        for (int i = 0; i < faces; ++i) {
            int flipFace = ((faceDirs >> (2 * i)) & 3) - 1;
            if (flipFace == 0 || !_meshChunk->should_have_quad(pos, i)) continue;

            std::vector<int> neighbours;
            if (_meshChunk->get_unique_neighbouring_vertices(pos, StitchedMeshChunk::FaceOffsets[i], neighbours) &&
                neighbours.size() == 4)
            {
                int n0 = _meshChunk->vertexIndices[neighbours[0]];
                int n1 = _meshChunk->vertexIndices[neighbours[1]];
                int n2 = _meshChunk->vertexIndices[neighbours[2]];
                int n3 = _meshChunk->vertexIndices[neighbours[3]];

                if (_verts[n0].distance_squared_to(_verts[n3]) < _verts[n1].distance_squared_to(_verts[n2])) {
                    add_tri(n0, n1, n3, flipFace == -1);
                    add_tri(n0, n3, n2, flipFace == -1);
                } else {
                    add_tri(n1, n3, n2, flipFace == -1);
                    add_tri(n1, n2, n0, flipFace == -1);
                }
            }
        }
    }
}

void StitchedMeshExtractor::stitch_edge_chunk_boundaries() {
    if (!_meshChunk->is_edge_chunk()) return;

    static const int faces = 3;
    static const glm::ivec3 offsets[3] = {
        glm::ivec3(1, 0, 0),
        glm::ivec3(0, 1, 0),
        glm::ivec3(0, 0, 1)
    };

    for (auto& [pos, node_id] : _innerEdgeNodes) {
        for (int i = 0; i < faces; ++i) {
            int innerNeighbour = node_id;
            if (!(_meshChunk->on_positive_edge(pos))) {
                auto it = _innerEdgeNodes.find(pos + offsets[i]);
                if (it == _innerEdgeNodes.end() || it->second < 0)
                    continue;
                innerNeighbour = it->second;
            }

            std::vector<std::vector<int>> neighboursLists = find_ring_nodes(pos, i);
            int n0 = _meshChunk->vertexIndices[node_id];
            int n1 = _meshChunk->vertexIndices[innerNeighbour];

            for (auto& neighbours : neighboursLists) {
                if (neighbours.size() == 2) {
                    int n2 = _meshChunk->vertexIndices[neighbours[0]];
                    int n3 = _meshChunk->vertexIndices[neighbours[1]];
                    if (_verts[n0].distance_squared_to(_verts[n3]) < _verts[n1].distance_squared_to(_verts[n2])) {
                        add_tri_fix_normal(n0, n1, n3);
                        add_tri_fix_normal(n0, n3, n2);
                    } else {
                        add_tri_fix_normal(n1, n3, n2);
                        add_tri_fix_normal(n1, n2, n0);
                    }
                } else if (neighbours.size() == 1) {
                    int n2 = _meshChunk->vertexIndices[neighbours[0]];
                    add_tri_fix_normal(n0, n1, n2);
                }
            }
        }
    }
}

ChunkMeshData* StitchedMeshExtractor::build_chunk_mesh_data(const JarVoxelTerrain& terrain) const {
    if (_indices.is_empty()) return nullptr;

    Array meshData;
    meshData.resize(Mesh::ARRAY_MAX);
    meshData[Mesh::ARRAY_VERTEX] = _verts;
    meshData[Mesh::ARRAY_NORMAL] = _normals;
    meshData[Mesh::ARRAY_COLOR]  = _colors;
    meshData[Mesh::ARRAY_INDEX]  = _indices;

    auto* output = new ChunkMeshData(
        meshData,
        _chunk->get_lod(),
        _meshChunk->is_edge_chunk(),
        _chunk->get_bounds(terrain.get_octree_scale())
    );

    output->boundaries = _meshChunk->_lodH2LBoundaries | (_meshChunk->_lodL2HBoundaries << 8);
    output->edgeVertices = _ringEdgeNodes;
    output->edgeVertices.insert(_innerEdgeNodes.begin(), _innerEdgeNodes.end());
    for (auto& [pos, node_id] : output->edgeVertices) {
        output->edgeVertices[pos] = _meshChunk->vertexIndices[node_id];
    }
    return output;
}
