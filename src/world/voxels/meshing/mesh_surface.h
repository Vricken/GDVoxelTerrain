#pragma once
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <unordered_map>

namespace godot
{

enum class WindingPolicy
{
    ForceCCW,       // Always CCW
    ForceCW,        // Always CW
    FromNormalCheck // Use cross product vs stored normal
};

class MeshSurface
{
  private:
    inline int add_vertex_from_main(int global_index, const MeshSurface &main)
    {
        auto it = g2l.find(global_index);
        if (it != g2l.end())
            return it->second;

        int local = add_vertex(main.verts[global_index], main.normals[global_index], main.colors[global_index]);
        g2l[global_index] = local;
        return local;
    }

    inline bool check_winding(int n0, int n1, int n2) const
    {
        Vector3 normal = (verts[n1] - verts[n0]).cross(verts[n2] - verts[n0]);
        return normal.dot(normals[n0]) < 0;
    }

    PackedVector3Array verts;
    PackedVector3Array normals;
    PackedColorArray colors;
    PackedInt32Array indices;

    // map from global index (main surface index) -> local index
    std::unordered_map<int, int> g2l;

  public:
    inline Array get_mesh_array() const
    {
        Array arr;
        arr.resize(Mesh::ARRAY_MAX);
        arr[Mesh::ARRAY_VERTEX] = verts;
        arr[Mesh::ARRAY_NORMAL] = normals;
        arr[Mesh::ARRAY_COLOR] = colors;
        arr[Mesh::ARRAY_INDEX] = indices;
        return arr;
    }

    inline bool no_vertices() const
    {
        return verts.is_empty();
    }
    inline bool no_indices() const
    {
        return indices.is_empty();
    }

    inline int add_vertex(const Vector3 &v, const Vector3 &n, const Color &c)
    {
        int local = verts.size();
        verts.push_back(v);
        normals.push_back(n);
        colors.push_back(c);
        return local;
    }

    // --- Triangle adders ---

    inline void add_triangle(int v0, int v1, int v2, WindingPolicy policy)
    {
        bool flip = false;
        switch (policy)
        {
        case WindingPolicy::ForceCW:
            flip = true;
            break;
        case WindingPolicy::FromNormalCheck:
            flip = !check_winding(v0, v1, v2);
            break;
        case WindingPolicy::ForceCCW:
        default:
            flip = false;
            break;
        }

        if (!flip)
        {
            indices.push_back(v0);
            indices.push_back(v1);
            indices.push_back(v2);
        }
        else
        {
            indices.push_back(v1);
            indices.push_back(v0);
            indices.push_back(v2);
        }
    }

    inline void add_triangle(int g0, int g1, int g2, const MeshSurface &main, WindingPolicy policy)
    {
        int l0 = add_vertex_from_main(g0, main);
        int l1 = add_vertex_from_main(g1, main);
        int l2 = add_vertex_from_main(g2, main);
        add_triangle(l0, l1, l2, policy);
    }

    // --- Quad adders ---

    inline void add_quad(int v0, int v1, int v2, int v3, WindingPolicy policy)
    {
        // Decide diagonal
        if (verts[v0].distance_squared_to(verts[v3]) < verts[v1].distance_squared_to(verts[v2]))
        {
            add_triangle(v0, v1, v3, policy);
            add_triangle(v0, v3, v2, policy);
        }
        else
        {
            add_triangle(v1, v3, v2, policy);
            add_triangle(v1, v2, v0, policy);
        }
    }

    inline void add_quad(int g0, int g1, int g2, int g3, const MeshSurface &main, WindingPolicy policy)
    {
        int l0 = add_vertex_from_main(g0, main);
        int l1 = add_vertex_from_main(g1, main);
        int l2 = add_vertex_from_main(g2, main);
        int l3 = add_vertex_from_main(g3, main);
        add_quad(l0, l1, l2, l3, policy);
    }
};

} // namespace godot
