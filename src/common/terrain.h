#pragma once

#include "memory.h"
#include "cpmath.h"
#include "pool_allocator.h"

#define CHUNK_WIDTH (8)
#define NOISE_SAMPLE_PER_CHUNK_WIDTH (1)
#define NODE_WIDTH (2)

/**
 * The LOD problem: How am I supposed to do LOD with 8x8x8 chunks?
 * Octree LOD is easy, but we're not using a pure octree.
 * Maybe I should "fake" the first two level of LOD.
 * Maybe I should also take the heightmap into account at higher LOD
 * That way, mountain lines will not be too ugly
 */

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
typedef Voxel Chunk[CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_WIDTH];

/**
 * 4x4x4x4 bytes aka TWO cache line
 * Contains 24 bit address to chunks and 8 bit voxel for far chunk LOD color
 * 24 bits means ~ 2**24 chunk address.
 * If the 8 bit voxel is 0xff, then the LOD color is "air"
 * Similarly an address of 0xff means no node/chunk
 * Since a chunk is 512 bytes, we can address ~ 8 Go RAM worth of chunks
 * If the 8 bit voxel is 0xff, it's an address to a Node or to air rather than a chunk
 */
typedef u16 Node[NODE_WIDTH*NODE_WIDTH*NODE_WIDTH];

typedef struct HeightApprox {
    u32 min;
    u32 max;
} HeightApprox;

typedef struct Terrain {

    // pool allocator that holds all 4x4x4 chunks and leaves
    PoolAllocator chunkPool;
    PoolAllocator nodePool;

    // pool address of the root node
    u32 root_node_address;

    // depth of the tree. 8 means 4**8 = 65 536 voxels aka 4096 MC chunks
    u32 depth;
    u32 width;
    u32 width_chunks;

    // heightmap. It's currently unused after world gen, but maybe someday we'll want to play with it.
    u32 *heightmap;
    HeightApprox **approx_heightmaps;

    // is set to true when the terrain has changed so its GPU-memory copy is updated.
    bool dirty;
} Terrain;

void terrain_init(Terrain* terrain, u32 depth);
void terrain_destroy(Terrain* terrain);
