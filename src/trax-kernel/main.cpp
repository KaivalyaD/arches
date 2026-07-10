#include "stdafx.hpp"
#include "include.hpp"
#include "custom-instr.hpp"
#include "intersect.hpp"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define IGNORE 0x0U

inline uint encode(float red, float green, float blue, float alpha = 1.0f)
{
    uint r = MIN(MAX(0.0f, red  ), 1.0f) * 0xff;
    uint g = MIN(MAX(0.0f, green), 1.0f) * 0xff;
    uint b = MIN(MAX(0.0f, blue ), 1.0f) * 0xff;
    uint a = MIN(MAX(0.0f, alpha), 1.0f) * 0xff;
    return r | (g << 8) | (b << 16) | (a << 24);
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
        // generate
        rtm::Ray ray = args.camera.generate_ray_through_pixel(frameX, frameY);
        
        // trace
        rtm::Hit hit(ray.t_max, rtm::vec2(0.0f), ~0U);
        _traceray<IGNORE>(IGNORE, ray, hit);

        // shade
        uint color = encode(0.0f, 0.0f, 0.0f); // miss
        if(hit.t < ray.t_max)
        {
            color = encode(1.0f, 0.0f, 0.0f); // hit
        }
        args.framebuffer[fb_index] = color;
    }
    return 0;
}
