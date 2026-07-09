#include "stdafx.hpp"
#include "include.hpp"
#include "custom-instr.hpp"
#include "intersect.hpp"

#define IGNORE 0U

inline uint encode(float red, float green, float blue, float alpha = 1.0f)
{
    uint r = fmax(fmin(red,   1.0f), 0.0f) * 0xff;
    uint g = fmax(fmin(green, 1.0f), 0.0f) * 0xff;
    uint b = fmax(fmin(blue,  1.0f), 0.0f) * 0xff;
    uint a = fmax(fmin(alpha, 1.0f), 0.0f) * 0xff;
    return r | (g << 8) | (b << 16) | (a << 24);
}

inline void raygen(float x, float y, rtm::Ray &ray)
{
    ray.o.x = x;
    ray.o.y = y;
    ray.o.z = 5.0f;
    ray.t_min = 0.1f;
    ray.d.x = x;
    ray.d.y = y;
    ray.d.z = -1.0f;
    ray.t_max = FLT_MAX;
}

int main(void)
{
    // framebuffer address format:
    // suppose framebuffer size is 256 x 256. Then, we get a 1-D index into framebuffer as:
    //    u32 index  = 0xYYXX
    // subdividing index, we can get 2-D image-space coords:
    //    u32 frameX = 0xXX
    //    u32 frameY = 0xYY
    // subdividing image-space coords, we can construct tiles with coords:
    //    u32 tileX  = frameX >> 2 // where 1 << 2 = 4 is the tile_width
    //    u32 tileY  = frameY >> 3 // where 1 << 3 = 8 is the tile_height
    // using tile and image-space coords, we can obtain coords local to the tile:
    //    u32 tile_local_x = frameX & 0b'0011 // 0b'0011 = ~(~0 << 2)
    //    u32 tile_local_y = frameY & 0b'0111 // 0b'0111 = ~(~0 << 3)
    constexpr uint TILE_X_INDEX_BITS  = 2;
    constexpr uint TILE_Y_INDEX_BITS  = 3;
    constexpr uint TILE_WIDTH         = 1 << TILE_X_INDEX_BITS;
    constexpr uint TILE_HEIGHT        = 1 << TILE_Y_INDEX_BITS;
    constexpr uint TILE_X_MASK        = (~0U << TILE_X_INDEX_BITS);
    constexpr uint TILE_Y_MASK        = (~0U << TILE_Y_INDEX_BITS);

    const TRaXKernelArgs args = *(TRaXKernelArgs *)(TRAX_KERNEL_ARGS_ADDRESS);
    for (uint index = fchthrd(); index < args.framebuffer_size; index = fchthrd())
	{
        uint frameX   = index % args.framebuffer_width;
        uint frameY   = index / args.framebuffer_width;
        uint fb_index = index;

        // trace a primary ray for each pixel within this tile
        rtm::Ray ray;
        float x = float(frameX) / float(args.framebuffer_width);
        float y = float(frameY) / float(args.framebuffer_height);
        raygen(x, y, ray);
        
        // rtm::Hit hit;
        // _traceray<IGNORE>(IGNORE, ray, hit);

        // uint color = 0xff'00'00'ff;
        // if(hit.t < ray.t_max)
        // {
        //     color = 0xff'ff'ff'ff;
        // }

        args.framebuffer[fb_index] = encode(x, 0.0f, 0.0f);
    }
    return 0;
}
