#ifndef WORLD_STORAGE_H
#define WORLD_STORAGE_H

#include "voxel_storage.h"
#include <string>
#include <vector>

namespace godot
{

// WorldStorage is a higher-level container that owns a VoxelStorage
// and knows where to persist it (path/filename).
class WorldStorage {
public:
    WorldStorage() = default;
    explicit WorldStorage(const std::string &path);

    void set_path(const std::string &path);
    const std::string &get_path() const;

    VoxelStorage &get_voxel_storage() { return voxel_storage; }
    const VoxelStorage &get_voxel_storage() const { return voxel_storage; }

    // In-memory serialization
    std::vector<uint8_t> serialize_to_buffer() const;
    void deserialize_from_buffer(const std::vector<uint8_t> &buffer);

    // File-based persistence
    bool write_to_file() const;
    bool load_from_file();

private:
    VoxelStorage voxel_storage;
    std::string save_path;
};

} // namespace godot

#endif // WORLD_STORAGE_H
