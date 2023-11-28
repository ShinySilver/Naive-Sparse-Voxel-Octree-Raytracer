#version 460
//#extension GL_ARB_shader_clock : enable
layout(local_size_x = 8,  local_size_y = 8) in;

layout(rgba8, binding = 0) uniform writeonly image2D outImage;

uniform uvec2 screenSize;
uniform uvec3 terrainSize;
uniform uvec3 treeDepth;
uniform vec3 camPos;
uniform mat4 viewMat;
uniform mat4 projMat;

layout(std430, binding = 0) readonly buffer node_pool
{
    uint nodePool[];
};

layout(std430, binding = 1) readonly buffer chunk_pool
{
    uint chunkPool[];
};

struct RayHit {
    vec3 hitPos; // position of the hit
    uint hitId; // id of the voxel that was hit
    uint faceId; // id of the face the was hit. can be used to determine the normal
};

// the face ID is returned as a single number, this array converts it to a normal
// (useful for passing the normal around / storing it in just 3 bits)
const vec3 normals[] = {
    vec3(-1,0,0),
    vec3(1,0,0),
    vec3(0,-1,0),
    vec3(0,1,0),
    vec3(0,0,-1),
    vec3(0,0,1)
};

RayHit intersectTerrain(vec3 rayPos, vec3 rayDir)
{
    // delta to avoid grid aligned rays
    if (rayDir.x == 0)
        rayDir.x = 0.001;
    if (rayDir.y == 0)
        rayDir.y = 0.001;
    if (rayDir.z == 0)
        rayDir.z = 0.001;
    rayDir = normalize(rayDir);

    return RayHit(vec3(0), 0, 0);
}

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
    RayHit hit = RayHit(vec3(0), 0, 0);

    // check if the camera is outside the voxel volume
    float intersect = AABBIntersect(vec3(0), vec3(terrainSize - 1), camPos, 1.0f / rayDir);

    // if it is outside, offset the ray so it is in the voxel volume
    if (intersect > 0) {
        // calc ray start pos
        rayPos += rayDir * (intersect + 0.001);
    }

    // intersect the ray agains the terrain if it crosses the terrain volume
    // vec3 colorTime = vec3(0);
    if (intersect >= 0) {
        // uvec2 start = clock2x32ARB();

        hit = intersectTerrain(rayPos, rayDir);

        // uvec2 end = clock2x32ARB();
        // uint time = end.x - start.x;
        // colorTime = vec3(time, 0, 0) / 1000000.0f;
    }

    // choose color (sky or voxel color)
    vec3 color = vec3(0.69, 0.88, 0.90);
    if (hit.hitId != 0) {
        vec3 normal = normals[hit.faceId - 1];

        // the color is packed as R3G3B2, a color palette would be preferable
        color = vec3((hit.hitId >> 5) / 7.0f, ((hit.hitId >> 2) & 7u) / 7.0f, (hit.hitId & 3u) / 3.0f);

        // simple normal based light
        color *= vec3(abs(dot(normal, normalize(vec3(1, 3, 1.5)))));
    }

    // output color to texture
    imageStore(outImage, ivec2(gl_GlobalInvocationID.xy), vec4(color, 1));
}
