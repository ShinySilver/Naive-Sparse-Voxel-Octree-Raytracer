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

// voxel palette. it mirrors materials.h
vec3 colors[] = {
vec3(1.00, 0.40, 0.40), // UNDEFINED
vec3(0.69, 0.88, 0.90), // AIR
vec3(0.33, 0.33, 0.33), // STONE
vec3(0.42, 0.32, 0.25), // DIRT
vec3(0.30, 0.39, 0.31)  // GRASS
};
vec3 debug_colors[] = {
vec3(1.0, 0.5, 0.5),
vec3(0.8, 0., 0.),
vec3(0.5, 1., 0.5),
vec3(0., 0.8, 0.),
vec3(0.5, 0.5, 1.),
vec3(0., 0., 0.8),
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

    // if it is outside the terrain, offset the ray so its starting position is (slightly) in the voxel volume
    if (intersect > 0) {
        rayPos += rayDir * (intersect + 0.001);
    }

    // if the ray intersect the terrain, raytrace
    vec3 color = vec3(0.69, 0.88, 0.90); // this is the sky color
    vec3 localRayPos = rayPos;

    if (intersect >= 0) {
        uint depth = 0;

        // at any time, node_width = terrain_width / NODE_WIDTH**depth
        uint node_width = terrainSize.x;

        // at any time, the top-most stack address is stack[depth]
        uint stack[12];

        // index of the current node in the pool
        uint current_node = 0;
        uint previous_node = 0;

        // color code of the last valid node
        uint color_code;

        // setting the stack to the starting pos of the ray
        do {
            depth += 1;
            node_width /= NODE_WIDTH;
            stack[depth] = current_node;
            uvec3 r = uvec3(localRayPos / node_width);
            uint node_data = nodePool[current_node * NODE_SIZE + r.x + r.z * NODE_WIDTH + r.y * NODE_WIDTH * NODE_WIDTH];
            localRayPos = localRayPos - r * node_width;
            previous_node = current_node;
            current_node = (node_data & 0x00ffffffu);
            color_code = (node_data >> 24);
        } while (current_node != 0 && depth < 32);

        // #######################################
        // # Unfinished code below, please ignore
        // #######################################

        if (false) {
            while (color_code == 1) {

                uvec3 mapPos = uvec3(floor(rayPos / node_width + 0.));
                vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
                vec3 sideDist = (sign(rayDir) * (vec3(mapPos) - rayPos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist;
                ivec3 rayStep = ivec3(sign(rayDir));
                bvec3 mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));

                //All components of mask are false except for the corresponding largest component of sideDist, which is the axis along which the ray should be incremented.
                sideDist += vec3(mask) * deltaDist;
                ivec3 step = ivec3(vec3(mask)) * rayStep * ivec3(node_width);

                while (any(lessThan(rayPos + step, rayPos - mod(rayPos, node_width))) || any(greaterThanEqual(rayPos + step, rayPos + node_width - mod(rayPos, node_width)))) {
                    node_width *= NODE_WIDTH;
                    depth -= 1;
                    current_node = stack[depth];
                }

                localRayPos = rayPos - mod(rayPos, node_width);
                while (current_node != 0 && depth < 32) {
                    depth += 1;
                    node_width /= NODE_WIDTH;
                    stack[depth] = current_node;
                    uvec3 r = uvec3(localRayPos / node_width);
                    uint node_data = nodePool[current_node * NODE_SIZE + r.x + r.z * NODE_WIDTH + r.y * NODE_WIDTH * NODE_WIDTH];
                    localRayPos = localRayPos - r * node_width;
                    current_node = (node_data & 0x00ffffffu);
                    color_code = (node_data >> 24);
                }
            }}

        // TODO finish the above code
        // TODO support chunk leafs (I'm scared)

        // ensuring the color code is valid
        if (color_code >= colors.length()) {
            color.xyz = colors[0].xyz;
        } else {
            // setting the pixel color using the color table
            //color.xyz = colors[color_code].xyz;

            // .. or using per-node color to debug the octree
            color.xyz = debug_colors[(previous_node >> 1) % debug_colors.length()].xyz;
        }
    }

    // output color to texture
    imageStore(outImage, ivec2(gl_GlobalInvocationID.xy), vec4(color, 1));
}
