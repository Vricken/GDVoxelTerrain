#include "world_storage.h"
#include <fstream>

namespace godot
{

WorldStorage::WorldStorage(const std::string &path) : save_path(path) {}

void WorldStorage::set_path(const std::string &path) {
    save_path = path;
}

const std::string &WorldStorage::get_path() const {
    return save_path;
}

std::vector<uint8_t> WorldStorage::serialize_to_buffer() const {
    return voxel_storage.serialize_to_buffer();
}

void WorldStorage::deserialize_from_buffer(const std::vector<uint8_t> &buffer) {
    voxel_storage.deserialize_from_buffer(buffer);
}

bool WorldStorage::write_to_file() const {
    if (save_path.empty()) return false;
    std::ofstream out(save_path, std::ios::binary);
    if (!out) return false;
    auto buffer = serialize_to_buffer();
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return true;
}

bool WorldStorage::load_from_file() {
    if (save_path.empty()) return false;
    std::ifstream in(save_path, std::ios::binary);
    if (!in) return false;
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
    deserialize_from_buffer(buffer);
    return true;
}

} // namespace godot
