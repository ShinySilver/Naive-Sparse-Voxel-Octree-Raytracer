#version 460

layout (local_size_x = 8, local_size_y = 8) in;

layout (rgba8, binding = 0) uniform writeonly image2D outImage;

uniform uvec2 screenSize;
uniform uvec3 terrainSize;
uniform uint treeDepth;
uniform vec3 camPos;
uniform mat4 viewMat;
uniform mat4 projMat;

#define NODE_WIDTH 2
#define NODE_SIZE NODE_WIDTH * NODE_WIDTH * NODE_WIDTH

layout (std430, binding = 0) readonly buffer node_pool
{
    uint nodePool[];
};

layout (std430, binding = 1) readonly buffer chunk_pool
{
    uint chunkPool[];
};

vec3 getRayDir(ivec2 screenPos)
{
    vec2 screenSpace = (screenPos + vec2(0.5)) / vec2(screenSize);
    vec4 clipSpace = vec4(screenSpace * 2.0f - 1.0f, -1.0, 1.0);
    vec4 eyeSpace = vec4(vec2(inverse(projMat) * clipSpace), -1.0, 0.0);
    return normalize(vec3(inverse(viewMat) * eyeSpace));
}

float AABBIntersect(vec3 bmin, vec3 bmax, vec3 orig, vec3 invdir)
{
    vec3 t0 = (bmin - orig) * invdir;
    vec3 t1 = (bmax - orig) * invdir;

    vec3 vmin = min(t0, t1);
    vec3 vmax = max(t0, t1);

    float tmin = max(vmin.x, max(vmin.y, vmin.z));
    float tmax = min(vmax.x, min(vmax.y, vmax.z));

    if (!(tmax < tmin) && (tmax >= 0))
    return max(0, tmin);
    return -1;
}

void main()
{
    // make sure current thread is inside the window bounds
    if (any(greaterThanEqual(gl_GlobalInvocationID.xy, screenSize)))
    return;

    // calc ray direction for current pixel
    vec3 rayDir = getRayDir(ivec2(gl_GlobalInvocationID.xy));
    vec3 rayPos = camPos;

    // check if the camera is outside the voxel volume
    float intersect = AABBIntersect(vec3(0), vec3(terrainSize - 1), camPos, 1.0f / rayDir);

    // if it is outside the terrain, offset the ray so its starting position is in the voxel volume
    if (intersect > 0) {
        rayPos += rayDir * (intersect + 0.001);
    }

    // if the ray intersect the terrain, raytrace
    vec3 color = vec3(0.69, 0.88, 0.90); // this is the sky color

    // colors used for debuging
    vec3 colors[] = {
    vec3(1.00, 0.40, 0.40), // UNDEFINED
    vec3(0.69, 0.88, 0.90), // AIR
    vec3(0.33, 0.33, 0.33), // STONE
    vec3(0.42, 0.32, 0.25), // DIRT
    vec3(0.30, 0.39, 0.31)  // GRASS
    };

    if (intersect >= 0) {
        uint depth = 0;

        // at any time, node_width = terrain_width / NODE_WIDTH**depth
        uint node_width = terrainSize.x;

        // at any time, the top-most stack address is stack[depth]
        uint stack[12];

        // index of the current node in the pool
        uint current_node = 0;

        // setting the stack to the starting pos of the ray
        do {
            depth += 1;
            node_width /= NODE_WIDTH;
            stack[depth] = current_node;
            uvec3 r = max(min(uvec3(rayPos/node_width), 1), 0);
            uint tmp = nodePool[current_node * NODE_SIZE
            + r.x
            + r.z * NODE_WIDTH
            + r.y * NODE_WIDTH * NODE_WIDTH];
            rayPos = rayPos - r*node_width;
            current_node = (tmp & 0x00ffffffu);

            // we only support 4 colors. If we receive an unsupported color, print white.
            uint color_code = (tmp>>24);
            if (color_code > 4) {
                color = vec3(1.0, 1.0, 1.0);
            } else color.xyz = colors[color_code].xyz;
        } while (current_node != 0 && depth < 8);

        if(current_node!=0){
            //color.xyz = colors[0].xyz;
        }

        // slightly coloring the sky in the octree
        //color.xyz *= 1 - 0.05*depth;
    }

    // output color to texture
    imageStore(outImage, ivec2(gl_GlobalInvocationID.xy), vec4(color, 1));
}
