#include <memory.h>
#include "terrain.h"
#include "cplog.h"
#include "pool_allocator.h"
#include "materials.h"

#define FNL_IMPL

#include "FastNoiseLite.h"
#include "log.h"
#include "cptime.h"

static fnl_state noiseGen2D;

static void terrain_generate(Terrain *terrain);

static void terrain_generate_chunk(Terrain *pTerrain, u32 x, u32 y, u32 z, Node (*node));

void terrain_init(Terrain *terrain, u32 depth) {
    if (depth <= 0) FATAL("Minimum SVO depth is 1");

    // info message
    terrain->width = CHUNK_WIDTH * (u32) pow(NODE_WIDTH, depth);
    terrain->width_chunks = (u32) pow(NODE_WIDTH, depth);
    terrain->depth = depth;
    char message[256];
    snprintf(message, 256,
             "SVO depth is set at %u. World is %ux%ux%u voxels", depth, terrain->width, terrain->width,
             terrain->width);
    INFO(message);

    // allocate ~128 Mo of RAM for each pool
    u32 initialPoolSize = 128 * 1024;
    poolAllocatorCreate(&terrain->chunkPool, initialPoolSize, sizeof(u32) * 512, NULL);
    poolAllocatorCreate(&terrain->nodePool, initialPoolSize, sizeof(u32) * 64, NULL);

    // Setup the worldgen noises
    srand(41233125);
    noiseGen2D = fnlCreateState();
    noiseGen2D.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noiseGen2D.fractal_type = FNL_FRACTAL_RIDGED;
    noiseGen2D.octaves = 3;
    noiseGen2D.seed = 41233125;
    noiseGen2D.frequency = 1;

    terrain->dirty = true;
    terrain_generate(terrain);
}

void terrain_destroy(Terrain *terrain) {
    poolAllocatorDestroy(&terrain->chunkPool);
    poolAllocatorDestroy(&terrain->nodePool);
    for (int i = 0; i <= terrain->depth; i++) {
        free(terrain->approx_heightmaps[i]);
    }
    free(terrain->approx_heightmaps);
    free(terrain->heightmap);
}

// Multiple res heightmap -> optimal SVO
// Later: low-res heightmap -> MinPool/MaxPool -> optimal SVO

static void terrain_generate_heightmap_recursive(Terrain *terrain, u32 width_chunks, HeightApprox **heightmaps,
                                                 u32 depth) {
    if (depth > terrain->depth) return;
    if (depth == 0) {
        // Create a heightmap at the right size
        heightmaps[0] = (HeightApprox *) malloc(width_chunks * width_chunks * sizeof(HeightApprox));
        if (!heightmaps[0]) FATAL("Out of memory.");
        INFO("Generating heightmap of size %ux%u, with one heightmap entry being one %ux%u chunk.",
             width_chunks, width_chunks, (u32) CHUNK_WIDTH, (u32) CHUNK_WIDTH);

        // For every chunk of the map...
        for (u32 cx = 0; cx < width_chunks; cx++) {
            for (u32 cy = 0; cy < width_chunks; cy++) {
                // Generate a precise heightmap but only keep a min and a max per chunk
                u32 min = UINT32_MAX, max = 0;
                for (u32 dx = 0; dx < CHUNK_WIDTH; dx++) {
                    for (u32 dy = 0; dy < CHUNK_WIDTH; dy++) {
                        u32 hx = cx * CHUNK_WIDTH + dx;
                        u32 hy = cy * CHUNK_WIDTH + dy;
                        u32 h = 0.25 * terrain->width +
                                0.5 * terrain->width * (fnlGetNoise2D(&noiseGen2D, hx * 0.005, hy * 0.005) * 0.5 + 0.5);
                        //h = 96;
                        terrain->heightmap[hx + (u64) terrain->width * hy] = h;
                        if (h < min) min = h;
                        if (h > max) max = h;
                    }
                }
                heightmaps[0][cx + cy * width_chunks] = (HeightApprox) {.min=min, .max=max};
            }
        }
    } else { // Aggregate fine-grained heightmaps into simplified heightmaps
        // Create a heightmap at the right size
        heightmaps[depth] = (HeightApprox *) malloc(width_chunks * width_chunks * sizeof(HeightApprox));
        if (!heightmaps[depth]) FATAL("Out of memory.");
        INFO("Generating sub-heightmap of size %ux%u with each heightmap entry corresponding to one %ux%u level %u node.",
             width_chunks, width_chunks, CHUNK_WIDTH * (u32) pow(NODE_WIDTH, depth),
             CHUNK_WIDTH * (u32) pow(NODE_WIDTH, depth),
             depth);

        // For every chunk of the map...
        for (u32 cx = 0; cx < width_chunks; cx++) {
            for (u32 cy = 0; cy < width_chunks; cy++) {
                u32 min = UINT32_MAX, max = 0;
                for (u32 dx = 0; dx < NODE_WIDTH; dx++) {
                    for (u32 dy = 0; dy < NODE_WIDTH; dy++) {
                        HeightApprox h = heightmaps[depth - 1][cx + dx + (cy + dy) * width_chunks * NODE_WIDTH];
                        if (h.min < min) min = h.min;
                        if (h.max > max) max = h.max;
                    }
                }
                // Generate a precise heightmap but only keep a min and a max per chunk
                heightmaps[depth][cx + cy * width_chunks] = (HeightApprox) {.min=min, .max=max};
            }
        }
    }
    terrain_generate_heightmap_recursive(terrain, width_chunks / NODE_WIDTH, heightmaps, depth + 1);
}

typedef struct SvoGenStats {
    u32 *empty_nodes_per_level;
    u32 *uniform_nodes_per_level;
    u32 *mixed_nodes_per_level;
} SvoGenStats;

static void terrain_generate_recursive(Terrain *terrain, u32 cx, u32 cy, u32 cz, u32 width_chunks, u32 depth,
                                       HeightApprox **approx_heightmaps, u32 node_address, SvoGenStats *stats) {
    depth -= 1;
    Node *node = poolAllocatorGet(&terrain->nodePool, node_address);
    for (u32 dx = 0; dx < NODE_WIDTH; dx++) {
        for (u32 dy = 0; dy < NODE_WIDTH; dy++) {
            HeightApprox height = approx_heightmaps[depth][cx + dx + (cy + dy) * width_chunks];
            for (u32 dz = 0; dz < NODE_WIDTH; dz++) {
                u32 coordinate_multiplier = CHUNK_WIDTH * (u32) pow(NODE_WIDTH, depth);
                if ((cz + dz + 1) * coordinate_multiplier <= height.min) {
                    // Node is made of pure stone
                    (*node)[dx + dy * NODE_WIDTH + dz * NODE_WIDTH * NODE_WIDTH] = STONE << 24;
                    stats->uniform_nodes_per_level[depth] += 1;
                } else if ((cz + dz) * coordinate_multiplier <= height.max) {
                    if (depth == 0) {
                        u32 chunk_id = poolAllocatorAlloc(&terrain->chunkPool);
                        node = poolAllocatorGet(&terrain->nodePool, node_address);
                        (*node)[dx + dy * NODE_WIDTH + dz * NODE_WIDTH * NODE_WIDTH] = (GRASS << 24) | chunk_id;
                        stats->mixed_nodes_per_level[depth] += 1;
                    } else {
                        // Chunk is made of a mix of air and stone
                        u32 subnode_id = poolAllocatorAlloc(&terrain->nodePool);
                        Node *new_node = poolAllocatorGet(&terrain->nodePool, subnode_id);

                        /**
                         * Actual chunk gen here!
                         * We use memset to roughly fill the chunk with AIR, and then place block per block.
                         * But this will be in another function :)
                         */
                        terrain_generate_chunk(terrain,
                                               (cx + dx) * coordinate_multiplier,
                                               (cy + dy) * coordinate_multiplier,
                                               (cz + dz) * coordinate_multiplier,
                                               new_node);

                        stats->mixed_nodes_per_level[depth] += 1;
                        terrain_generate_recursive(terrain,
                                                   (cx + dx) * NODE_WIDTH,
                                                   (cy + dy) * NODE_WIDTH,
                                                   (cz + dz) * NODE_WIDTH,
                                                   width_chunks * NODE_WIDTH,
                                                   depth,
                                                   approx_heightmaps,
                                                   subnode_id,
                                                   stats);
                        node = poolAllocatorGet(&terrain->nodePool, node_address);
                        (*node)[dx + dy * NODE_WIDTH + dz * NODE_WIDTH * NODE_WIDTH] = (GRASS << 24) | subnode_id;
                    }
                } else {
                    // else, node is made of air, do nothing. 0x0 means an air chunk.
                    stats->empty_nodes_per_level[depth] += 1;
                }
            }
        }
    }

}

static void terrain_generate_chunk(Terrain *pTerrain, u32 x, u32 y, u32 z, Node (*node)) {

}

static void terrain_generate(Terrain *terrain) {
    // issue: we want bot to top heightmap to have a real min but top to bot gen to have minimum terrain
    // solution: generate them separately, starting with the heightmap at chunk res!

    /**
     * Generating the heightmaps using a first recursive function
     */
    u32 time = uclock();
    terrain->approx_heightmaps = (u32 **) malloc((terrain->depth + 1) * sizeof(u32 *));
    terrain->heightmap = (u32 *) malloc(terrain->width * terrain->width * sizeof(u32));
    if (!terrain->heightmap | !terrain->approx_heightmaps) FATAL("Out of memory.");
    terrain_generate_heightmap_recursive(terrain, terrain->width_chunks, terrain->approx_heightmaps, 0);
    INFO("Generating heightmaps took %.2fms. Min height is %u, max height is %u.", (uclock() - time) / 1e3,
         terrain->approx_heightmaps[terrain->depth][0].min, terrain->approx_heightmaps[terrain->depth][0].max);

    time = uclock(); // resetting the timer in order to get the SVO generation time

    /**
     * Creating the struct that will be used to get back tasty stats from the SVO generation recursive function
     */
    SvoGenStats stats = (SvoGenStats) {.empty_nodes_per_level=(u32 *) malloc(terrain->depth * sizeof(u32)),
            .mixed_nodes_per_level=(u32 *) malloc(terrain->depth * sizeof(u32)),
            .uniform_nodes_per_level=(u32 *) malloc(terrain->depth * sizeof(u32))};
    if (!stats.empty_nodes_per_level || !stats.mixed_nodes_per_level || !stats.uniform_nodes_per_level) FATAL(
            "Out of memory.");
    memset(stats.empty_nodes_per_level, 0, terrain->depth * sizeof(u32));
    memset(stats.mixed_nodes_per_level, 0, terrain->depth * sizeof(u32));
    memset(stats.uniform_nodes_per_level, 0, terrain->depth * sizeof(u32));

    /**
     * Creating the root node, and feeding it to the recursive function to create its leaves
     */
    terrain->root_node_address = poolAllocatorAlloc(&terrain->nodePool);
    terrain_generate_recursive(terrain, 0, 0, 0, NODE_WIDTH, terrain->depth, terrain->approx_heightmaps,
                               terrain->root_node_address,
                               &stats);
    for (u16 i = terrain->depth - 1; i >= 0 && i < terrain->depth; i--) {
        INFO("SVO level %u contains %u air nodes, %u uniform non-air nodes and %u %s.", i,
             stats.empty_nodes_per_level[i], stats.uniform_nodes_per_level[i], stats.mixed_nodes_per_level[i],
             i == 0 ? "chunks" : "mixed nodes");
    }
    free(stats.mixed_nodes_per_level);
    free(stats.uniform_nodes_per_level);
    free(stats.empty_nodes_per_level);
    INFO("Generating SVO from heightmaps took %.2fms", (uclock() - time) / 1e3);
}
