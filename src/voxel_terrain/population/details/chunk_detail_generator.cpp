#include "chunk_detail_generator.h"
#include "extracted_mesh_data.h"
#include "voxel_terrain.h"
#include "world.h"

using namespace godot;

ChunkDetailGenerator::ChunkDetailGenerator(JarWorld *world, VoxelMaterialMode material_mode)
    : world(world), material_mode(material_mode)
{
    // Constructor implementation
    // load details
}

ChunkDetailGenerator::~ChunkDetailGenerator()
{
    // Destructor implementation
}

Vector3 ChunkDetailGenerator::get_gravity_normal(const Vector3 &position) const
{
    if (world != nullptr)
        return world->get_gravity_vector(position).normalized();

    return Vector3(0, -1, 0);
}

float ChunkDetailGenerator::get_height(const Vector3 &position) const
{
    if (world != nullptr)
        return world->get_height(position);

    return position.y;
}

uint32_t ChunkDetailGenerator::get_material_index_from_triangle(const Triangle &tri, const std::vector<Color> &colors, const glm::vec2 &barycentric) const
{
    if (material_mode == VoxelMaterialMode::VOXEL_MATERIAL_MODE_MULTIPLE_MESHES)
        return tri.material_index;

    Color color = colors[tri.a] + barycentric.x * (colors[tri.b] - colors[tri.a]) +
                barycentric.y * (colors[tri.c] - colors[tri.a]);

    return VoxelTerrainMaterial::get_material_from_color(material_mode, VoxelTerrainMaterial::to_vec4(color));
}

TypedArray<MultiMesh> ChunkDetailGenerator::generate_details(const TypedArray<JarTerrainDetail> &details,
                                                             const ExtractedMeshData &chunkMeshData)
{
    std::vector<Vector3> all_verts;
    std::vector<Vector3> all_normals;
    std::vector<Color> all_colors;
    std::vector<Triangle> triangles;

    // --- Build global arrays and triangles ---
    for (size_t s = 0; s < chunkMeshData.mesh_arrays.size(); ++s)
    {
        const Array &mesh_array = chunkMeshData.mesh_arrays[s];
        int mat_index = chunkMeshData.material_indices[s];

        PackedVector3Array verts = mesh_array[Mesh::ARRAY_VERTEX];
        PackedVector3Array normals = mesh_array[Mesh::ARRAY_NORMAL];
        PackedColorArray colors = mesh_array[Mesh::ARRAY_COLOR];
        PackedInt32Array indices = mesh_array[Mesh::ARRAY_INDEX];

        int base = all_verts.size();

        // append vertices/normals/colors into global arrays
        for (int i = 0; i < verts.size(); i++)
        {
            all_verts.push_back(verts[i]);
            all_normals.push_back(normals[i]);
            all_colors.push_back(colors[i]);
        }

        // build triangles
        for (int i = 0; i < indices.size(); i += 3)
        {
            Triangle tri;
            tri.a = base + indices[i];
            tri.b = base + indices[i + 1];
            tri.c = base + indices[i + 2];
            tri.material_index = mat_index;
            triangles.push_back(tri);
        }
    }

    glm::vec3 chunkCenterGLM = chunkMeshData.bounds.get_center();
    Vector3 chunkCenter(chunkCenterGLM.x, chunkCenterGLM.y, chunkCenterGLM.z);

    if (details.is_empty())
        return TypedArray<MultiMesh>();

    std::vector<std::vector<Transform3D>> ts(details.size());

    // --- Iterate triangles ---
    for (const Triangle &tri : triangles)
    {
        Vector3 posA = all_verts[tri.a];
        Vector3 posB = all_verts[tri.b];
        Vector3 posC = all_verts[tri.c];

        Vector3 edge1 = posB - posA;
        Vector3 edge2 = posC - posA;
        float area = edge1.cross(edge2).length() * 0.5f;

        // compute interpolated normal/color as before
        for (size_t d = 0; d < details.size(); d++)
        {
            Ref<JarTerrainDetail> detail = details[d];

            // material filtering
            if (!detail->supports_material(tri.material_index))
                continue;

            float n = area * detail->get_density();
            int nInt = static_cast<int>(std::floor(n));
            float nFloat = DeterministicFloat((posA + posB + posC) * (1 + d));
            nInt += (nFloat < (n - nInt)) ? 1 : 0;

            for (int j = 0; j < nInt; j++)
            {
                float r1 = DeterministicFloat(posA * (1 + d));
                float r2 = DeterministicFloat(posB * (1 + d));
                if (r1 + r2 >= 1)
                {
                    r1 = 1 - r1;
                    r2 = 1 - r2;
                }

                Vector3 position = posA + r1 * edge1 + r2 * edge2;
                Vector3 worldPosition = position + chunkCenter;
                Vector3 world_up = -get_gravity_normal(worldPosition);
                float height = get_height(worldPosition);
                if (!detail->is_height_in_range(height))
                    continue;

                Vector3 up = all_normals[tri.a] + r1 * (all_normals[tri.b] - all_normals[tri.a]) +
                             r2 * (all_normals[tri.c] - all_normals[tri.a]);

                uint32_t material_index = get_material_index_from_triangle(tri, all_colors, glm::vec2(r1, r2));

                if(!detail->supports_material(material_index))
                    continue;

                up = up.normalized();

                float dot = up.dot(world_up);
                if (detail->is_slope_in_range(dot))
                    continue;

                if (!detail->get_align_with_normal())
                    up = world_up;

                float r3 = DeterministicFloat(posC * (1 + d));
                float r4 = DeterministicFloat((posA + posC) * (1 + d));

                ts[d].emplace_back(
                    detail->build_transform(position, up, r3 * 2 * Math_PI,
                                            Math::lerp(detail->get_minimum_scale(), detail->get_maximum_scale(), r4)));
            }
        }
    }

    TypedArray<MultiMesh> meshes;
    for (size_t d = 0; d < details.size(); d++)
    {
        Ref<JarTerrainDetail> detail = details[d];
        Ref<MultiMesh> mesh = memnew(MultiMesh());
        mesh->set_transform_format(MultiMesh::TRANSFORM_3D);
        mesh->set_mesh(detail->get_mesh());
        mesh->set_instance_count(ts[d].size());
        for (size_t i = 0; i < ts[d].size(); i++)
            mesh->set_instance_transform(i, ts[d][i]);
        meshes.append(mesh);
    }
    return meshes;
}
