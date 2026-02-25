#ifndef MESH_COMPUTE_SCHEDULER_H
#define MESH_COMPUTE_SCHEDULER_H

#include "voxel_octree_node.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <thread>
#include "utility/thread_pool.h"

using namespace godot;

class JarVoxelTerrain;

// ---------------------------------------------------------------------------
// Cross-platform replacement for concurrency::concurrent_queue (MS PPL).
// Wraps std::queue with a std::mutex and exposes the same push/try_pop/empty
// interface that the original code relied on.
// ---------------------------------------------------------------------------
template <typename T>
class ConcurrentQueue {
public:
    void push(const T &value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(value);
    }

    // Returns true and sets 'out' if the queue was non-empty; false otherwise.
    bool try_pop(T &out) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) return false;
        out = std::move(_queue.front());
        _queue.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

private:
    std::queue<T>       _queue;
    mutable std::mutex  _mutex;
};

// ---------------------------------------------------------------------------
// Cross-platform replacement for concurrency::concurrent_priority_queue.
// Wraps std::priority_queue with a std::mutex and exposes the same
// push/try_pop/empty interface.
// ---------------------------------------------------------------------------
template <typename T, typename Comparator = std::less<T>>
class ConcurrentPriorityQueue {
public:
    void push(const T &value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _pq.push(value);
    }

    // Returns true and sets 'out' if the queue was non-empty; false otherwise.
    bool try_pop(T &out) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_pq.empty()) return false;
        out = _pq.top();
        _pq.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _pq.empty();
    }

private:
    std::priority_queue<T, std::vector<T>, Comparator> _pq;
    mutable std::mutex                                  _mutex;
};

// ---------------------------------------------------------------------------

struct ChunkComparator {
    bool operator()(const VoxelOctreeNode *a, const VoxelOctreeNode *b) const {
        return a->get_lod() > b->get_lod();
    }
};

class MeshComputeScheduler
{
  private:
    ConcurrentPriorityQueue<VoxelOctreeNode*, ChunkComparator>          ChunksToAdd;
    ConcurrentQueue<std::pair<VoxelOctreeNode*, ChunkMeshData*>>        ChunksToProcess;

    std::atomic<int> _activeTasks;
    int _maxConcurrentTasks;

    ThreadPool threadPool;

    // Debug variables
    int _totalTris;
    int _prevTris;

    void process_queue(JarVoxelTerrain &terrain);
    void run_task(const JarVoxelTerrain &terrain, VoxelOctreeNode &chunk);

  public:
    MeshComputeScheduler(int maxConcurrentTasks);
    void enqueue(VoxelOctreeNode &node);
    void process(JarVoxelTerrain &terrain);
    void clear_queue();

    bool is_meshing()
    {
        return !ChunksToAdd.empty();
    }
};

#endif // MESH_COMPUTE_SCHEDULER_H
