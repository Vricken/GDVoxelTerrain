#ifndef VOXEL_STORAGE_H
#define VOXEL_STORAGE_H

#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include "voxel_octree_node.h"

namespace godot
{


class VoxelStorageTreeNode {
public:
    virtual ~VoxelStorageTreeNode() = default;
    virtual bool is_leaf() const = 0;

    // Serialize into a byte buffer
    virtual void serialize(std::vector<uint8_t> &out) const = 0;
    // Deserialize from a byte buffer (cursor advanced by reference)
    virtual void deserialize(const std::vector<uint8_t> &in, size_t &cursor) = 0;
};

class VoxelStorageInnerNode : public VoxelStorageTreeNode {
public:
    VoxelStorageInnerNode() = default;
    VoxelStorageInnerNode(VoxelOctreeNode* node);
    ~VoxelStorageInnerNode() override = default;

    bool is_leaf() const override { return false; }

    std::array<std::unique_ptr<VoxelStorageTreeNode>, 8> &get_children() { return children; }

    void serialize(std::vector<uint8_t> &out) const override;
    void deserialize(const std::vector<uint8_t> &in, size_t &cursor) override;

private:
    std::array<std::unique_ptr<VoxelStorageTreeNode>, 8> children;
};

class VoxelStorageLeafNode : public VoxelStorageTreeNode {
public:
    VoxelStorageLeafNode() = default;
    VoxelStorageLeafNode(VoxelOctreeNode* node);
    ~VoxelStorageLeafNode() override = default;

    bool is_leaf() const override { return true; }

    void serialize(std::vector<uint8_t> &out) const override;
    void deserialize(const std::vector<uint8_t> &in, size_t &cursor) override;

    float value = 0.0f;
    glm::vec3 normal{0.0f};
    uint32_t material_index = 0;
};

class VoxelStorageUneditedLeafNode : public VoxelStorageTreeNode {
public:
    bool is_leaf() const override { return true; }
    void serialize(std::vector<uint8_t> &out) const override;
    void deserialize(const std::vector<uint8_t> &in, size_t &cursor) override;
};

class VoxelStorage {
public:
    VoxelStorage() = default;
    ~VoxelStorage() = default;

    void build_from(VoxelOctreeNode* root); // TODO
    VoxelStorageTreeNode* get_root() const { return _root.get(); }

    // In‑memory serialization
    std::vector<uint8_t> serialize_to_buffer() const;
    void deserialize_from_buffer(const std::vector<uint8_t> &buffer);

private:
    std::unique_ptr<VoxelStorageTreeNode> _root;
};

} // namespace godot

#endif // VOXEL_STORAGE_H
