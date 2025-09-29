#ifndef VOXEL_MATERIAL_H
#define VOXEL_MATERIAL_H 

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/core/binder_common.hpp>
 
// DiscreteChannelSplatting:
//   Encodes material index as a one-hot RGBA vector.
//   Each material maps to a unique channel (e.g. red, green, blue, alpha).
//   Simple and cheap, but limited to 5 materials per draw call. (4 channels + 1 for no material)


// MeshPerMaterial:
//   Splits geometry into separate meshes per material.
//   Each mesh is assigned its own material and rendered independently.
//   Clean and flexible, but increases draw calls and batching complexity.


// PackedColorMaterial:
//   Treats the uint32_t material index as packed RGBA bytes.
//   Each channel stores an 8-bit value, interpreted as normalized float.
//   Useful for encoding up to 4 parameters per voxel (e.g. roughness, blend, ID).


// IndexedBlendSplatting:
//   Encodes two material indices (a, b) and a blend factor in RGB.
//   Allows smooth interpolation between materials using vertex colors.
//   Ideal for terrain blending, layered surfaces, and stylized transitions.

enum VoxelMaterialMode {
    VOXEL_MATERIAL_MODE_DISCRETE_CHANNEL_SPLATTING,
    VOXEL_MATERIAL_MODE_MULTIPLE_MESHES,
    VOXEL_MATERIAL_MODE_PACKED_COLOR,
    VOXEL_MATERIAL_MODE_INDEXED_BLEND_SPLATTING
};


namespace VoxelTerrainMaterial {

const float VOXEL_CHANNEL_SPLATTING_THRESHOLD = 0.5f;

inline glm::vec4 to_vec4(const godot::Color &c) {
    return glm::vec4(c.r, c.g, c.b, c.a);
}

inline godot::Color to_color(const glm::vec4 &v) {
    return godot::Color(v.r, v.g, v.b, v.a);
}

// Convert packed uint32_t to Color (each channel normalized to [0,1])
inline glm::vec4 packed_index_to_color(uint32_t index) {
    float r = float((index >> 24) & 0xFF) / 255.0f;
    float g = float((index >> 16) & 0xFF) / 255.0f;
    float b = float((index >> 8)  & 0xFF) / 255.0f;
    float a = float((index >> 0)  & 0xFF) / 255.0f;
    return {r, g, b, a};
}

// Convert godot::Color to packed uint32_t (each channel quantized to 8 bits)
inline uint32_t color_to_packed_index(const glm::vec4 &color) {
    uint32_t r = uint32_t(godot::Math::clamp(color.r * 255.0f, 0.0f, 255.0f));
    uint32_t g = uint32_t(godot::Math::clamp(color.g * 255.0f, 0.0f, 255.0f));
    uint32_t b = uint32_t(godot::Math::clamp(color.b * 255.0f, 0.0f, 255.0f));
    uint32_t a = uint32_t(godot::Math::clamp(color.a * 255.0f, 0.0f, 255.0f));
    return (r << 24) | (g << 16) | (b << 8) | a;
}

inline glm::vec4 get_color_from_material(const VoxelMaterialMode mode, const uint32_t material_index)
{
    switch (mode)
    {
    case VOXEL_MATERIAL_MODE_DISCRETE_CHANNEL_SPLATTING:
        {
            switch (material_index % 5)
            {
            case 0:
                return glm::vec4(0, 0, 0, 0);
            case 1:
                return glm::vec4(1, 0, 0, 0);
            case 2:
                return glm::vec4(0, 1, 0, 0);
            case 3:
                return glm::vec4(0, 0, 1, 0);
            case 4:
                return glm::vec4(0, 0, 0, 1);
            }
            break;
        }
        case VOXEL_MATERIAL_MODE_PACKED_COLOR:
        {
            auto color =  VoxelTerrainMaterial::packed_index_to_color(material_index);
            return glm::vec4(color.r, color.g, color.b, color.a);
        }    

    }
    return glm::vec4(0,0,0,0);
}

inline uint32_t get_material_from_color(const VoxelMaterialMode mode, const glm::vec4 color)
{
    switch (mode)
    {
    case VOXEL_MATERIAL_MODE_DISCRETE_CHANNEL_SPLATTING:
        {
            for(int material_index = 0; material_index < 4; material_index++)
                if (color[material_index] > VOXEL_CHANNEL_SPLATTING_THRESHOLD)
                    return material_index + 1;
            return 0;
        }
        case VOXEL_MATERIAL_MODE_PACKED_COLOR:
        {
            return VoxelTerrainMaterial::color_to_packed_index(color);
        }    

    }
    return 0;
}

}

VARIANT_ENUM_CAST(VoxelMaterialMode);

#endif // VOXEL_MATERIAL_H