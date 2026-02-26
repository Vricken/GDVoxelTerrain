#ifndef MESH_COMPUTE_SCHEDULER_H
#define MESH_COMPUTE_SCHEDULER_H

#include "concurrentqueue.h" // moodycamel lock-free queue
#include "utility/thread_pool.h"
#include "voxel_octree_node.h"
#include <atomic>
#include <functional>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
// <mutex> removed — ConcurrentPriorityQueue replaced with plain
// std::priority_queue
#include <queue> // std::priority_queue (ChunksToAdd) + std::queue used by moodycamel internally
#include <thread>
#include <utility>
#include <vector>

using namespace godot;

class JarVoxelTerrain;

// ---------------------------------------------------------------------------
// ConcurrentQueue — lock-free MPMC queue via moodycamel::ConcurrentQueue.
// Provides the same push/try_pop/empty interface as the original MS PPL shim
// but without any mutex; producers and consumers never block each other.
// ---------------------------------------------------------------------------
template <typename T> class ConcurrentQueue {
public:
  void push(const T &value) { _q.enqueue(value); }

  // Returns true and sets 'out' if the queue was non-empty; false otherwise.
  bool try_pop(T &out) { return _q.try_dequeue(out); }

  // NOTE: size_approx() is O(1) but approximate; treat as a hint only.
  bool empty() const { return _q.size_approx() == 0; }

private:
  moodycamel::ConcurrentQueue<T> _q;
};

// ConcurrentPriorityQueue removed: both push (enqueue) and pop (process_queue)
// are called from the same main/Godot thread, so no synchronisation is needed.
// ChunksToAdd is now a plain std::priority_queue (see MeshComputeScheduler
// below).

// ---------------------------------------------------------------------------

struct ChunkComparator {
  bool operator()(const VoxelOctreeNode *a, const VoxelOctreeNode *b) const {
    return a->get_lod() > b->get_lod();
  }
};

class MeshComputeScheduler {
private:
  // Single-threaded (main thread only) — plain priority_queue, no locking
  // needed.
  std::priority_queue<VoxelOctreeNode *, std::vector<VoxelOctreeNode *>,
                      ChunkComparator>
      ChunksToAdd;
  ConcurrentQueue<std::pair<VoxelOctreeNode *, ChunkMeshData *>>
      ChunksToProcess;

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

  bool is_meshing() { return !ChunksToAdd.empty(); }
};

#endif // MESH_COMPUTE_SCHEDULER_H
