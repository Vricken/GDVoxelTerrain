#include "voxel_storage.h"
#include <cstring>

namespace godot
{

// Utility to append raw bytes
template<typename T>
static void append_bytes(std::vector<uint8_t> &out, const T &val) {
    const uint8_t *ptr = reinterpret_cast<const uint8_t*>(&val);
    out.insert(out.end(), ptr, ptr + sizeof(T));
}

template<typename T>
static void read_bytes(const std::vector<uint8_t> &in, size_t &cursor, T &val) {
    std::memcpy(&val, in.data() + cursor, sizeof(T));
    cursor += sizeof(T);
}


// --- VoxelStorageInnerNode ---
void VoxelStorageInnerNode::serialize(std::vector<uint8_t> &out) const {
    uint8_t type = 1;
    append_bytes(out, type);
    for (const auto &child : children) {
        if (child) {
            child->serialize(out);
        } else {
            uint8_t empty = 0;
            append_bytes(out, empty);
        }
    }
}

void VoxelStorageInnerNode::deserialize(const std::vector<uint8_t> &in, size_t &cursor) {
    for (auto &child : children) {
        uint8_t type;
        read_bytes(in, cursor, type);
        if (type == 0) {
            child = std::make_unique<VoxelStorageUneditedLeafNode>();
        } else if (type == 1) {
            auto inner = std::make_unique<VoxelStorageInnerNode>();
            inner->deserialize(in, cursor);
            child = std::move(inner);
        } else if (type == 2) {
            auto leaf = std::make_unique<VoxelStorageLeafNode>();
            leaf->deserialize(in, cursor);
            child = std::move(leaf);
        }
    }
}

VoxelStorageInnerNode::VoxelStorageInnerNode(VoxelOctreeNode *node)
{
    if (!node || node->is_leaf()) return;

    for (size_t i = 0; i < 8; ++i) {
        VoxelOctreeNode *data_child = (*node->get_children())[i].get();
        if (!data_child->is_modified()) {
            this->children[i] = std::make_unique<VoxelStorageUneditedLeafNode>();
            continue;
        } else if (data_child->is_leaf()) {
            this->children[i] = std::make_unique<VoxelStorageLeafNode>(data_child);
            continue;
        }
        this->children[i] = std::make_unique<VoxelStorageInnerNode>(data_child);
    }
}

VoxelStorageLeafNode::VoxelStorageLeafNode(VoxelOctreeNode *node)
{
    if (node) {
        value = node->get_value();
        normal = node->get_normal();
        material_index = node->get_material_index();
    }
}

// --- VoxelStorageLeafNode ---
void VoxelStorageLeafNode::serialize(std::vector<uint8_t> &out) const {
    uint8_t type = 2;
    append_bytes(out, type);
    append_bytes(out, value);
    append_bytes(out, normal);
    append_bytes(out, material_index);
}

void VoxelStorageLeafNode::deserialize(const std::vector<uint8_t> &in, size_t &cursor) {
    read_bytes(in, cursor, value);
    read_bytes(in, cursor, normal);
    read_bytes(in, cursor, material_index);
}

// --- VoxelStorageUneditedLeafNode ---
void VoxelStorageUneditedLeafNode::serialize(std::vector<uint8_t> &out) const {
    uint8_t type = 0;
    append_bytes(out, type);
}
void VoxelStorageUneditedLeafNode::deserialize(const std::vector<uint8_t> &, size_t &) {
    // nothing
}

void VoxelStorage::build_from(VoxelOctreeNode *root)
{
    if (!root || !root->is_modified())
    _root = std::make_unique<VoxelStorageUneditedLeafNode>();
    else if (root->is_leaf()) {
        _root = std::make_unique<VoxelStorageLeafNode>(root);
    } else {
        _root = std::make_unique<VoxelStorageInnerNode>(root);
    }
}

// --- VoxelStorage ---
std::vector<uint8_t> VoxelStorage::serialize_to_buffer() const {
    std::vector<uint8_t> buffer;
    if (_root) _root->serialize(buffer);
    return buffer;
}

void VoxelStorage::deserialize_from_buffer(const std::vector<uint8_t> &buffer) {
    size_t cursor = 0;
    uint8_t type;
    read_bytes(buffer, cursor, type);
    if (type == 0) {
        _root = std::make_unique<VoxelStorageUneditedLeafNode>();
    } else if (type == 1) {
        _root = std::make_unique<VoxelStorageInnerNode>();
        _root->deserialize(buffer, cursor);
    } else if (type == 2) {
        auto leaf = std::make_unique<VoxelStorageLeafNode>();
        leaf->deserialize(buffer, cursor);
        _root = std::move(leaf);
    }
}

} // namespace godot
