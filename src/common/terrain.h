#pragma once

#include "memory.h"
#include "cpmath.h"
#include "pool_allocator.h"

/**
 * A voxel is 8 bits, 1 bytes.
 * 8 bits palette
 * Because of this format:
 * - palette supports 256 different voxels
 * - eventual state values will be in a separated hashmap (later)
 *
 * @note 2 bytes voxels and 2x2x2 1-cache-line chunks is also possible
 */
typedef u8 Voxel;

/**
 * 8x8x8x1 bytes aka FOUR cache lines
 * Contains voxels by value.
 */
typedef Voxel Chunk[512];

/**
 * 4x4x4x4 bytes aka TWO cache line
 * Contains 24 bit address to chunks and 8 bit voxel for far chunk LOD color
 * 24 bits means ~ 2**24 chunk address.
 * Since a chunk is 512 bytes, we can address ~ 8 Go RAM worth of chunks
 * If the 8 bit voxel is 0xff, it's an address to a Node or to air rather than a chunk
 */
typedef u32 Node[64];

typedef struct HeightApprox {
    u32 min;
    u32 max;
} HeightApprox;

typedef struct Terrain {
    // top level array holding info about each 8x8x8 chunk:
    // leading 0 : chunk is filled uniformly and the remaining 15 bits are Voxel bits. 16 unused bits.
    // leading 1 : chunk is not empty and the next 31 bits are the index into the chunk pool
    // Ox0 : chunk is empty

    // pool allocator that holds all 4x4x4 chunks and leaves
    PoolAllocator chunkPool;
    PoolAllocator nodePool;

    // pool address of the root node
    u32 root_node_address;

    // depth of the tree. 8 means 4**8 = 65 536 voxels aka 4096 MC chunks
    u32 depth;
    u32 width;
    u32 width_chunks;

    // tmp
    u32 *heightmap;
    HeightApprox **approx_heightmaps;


    bool dirty;
} Terrain;

void terrain_init(Terrain* terrain, u32 depth);
void terrain_destroy(Terrain* terrain);
