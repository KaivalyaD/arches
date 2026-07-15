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

inline void map_thread_into_pixel(
    uint thread_id,
    uint tile_id,
    uint framebuffer_width,
    uint tile_width,
    uint tile_height,
    uint &pixel_x,
    uint &pixel_y,
    uint &tile_x,
    uint &tile_y
) {
    tile_x  = tile_id % (framebuffer_width / tile_width);
    tile_y  = tile_id / (framebuffer_width / tile_width);
    pixel_x = tile_x * tile_width + thread_id % tile_width;
    pixel_y = tile_y * tile_height + thread_id / tile_width;
}

int main(void)
{
    constexpr uint TILE_WIDTH  = 4;
    constexpr uint TILE_HEIGHT = 8;
    constexpr uint TILE_SIZE   = TILE_WIDTH * TILE_HEIGHT;

    const TRaXKernelArgs args = *(TRaXKernelArgs *)(TRAX_KERNEL_ARGS_ADDRESS);
    for (uint index = fchthrd(); index < args.framebuffer_size; index = fchthrd())
	{
        uint tileId   = index / TILE_SIZE;
        uint threadId = index % TILE_SIZE;
        // uint tile_x, tile_y, x, y;
        // map_thread_into_pixel(threadId, tileId, args.framebuffer_width, TILE_WIDTH, TILE_HEIGHT, x, y, tile_x, tile_y);
		// uint fb_index = y * args.framebuffer_width + x;
		uint x = index % args.framebuffer_width;
        uint y = index / args.framebuffer_width;
        uint fb_index = index;

        // trace a primary ray for each pixel within this tile
        // generate
        rtm::Ray ray = args.camera.generate_ray_through_pixel(x, y);
        
        // trace
        rtm::Hit hit(ray.t_max, rtm::vec2(0.0f), ~0U);
        _traceray<IGNORE>(IGNORE, ray, hit);

        // shade
        uint color = encode(0.3f, 0.3f, 0.3f); // miss
        // if(hit.t < ray.t_max)
        // {
            // color = encode(1.0f, 0.0f, 0.0f); // hit
            // if(((x >> 2) & 0x1) ^ !((y >> 3) & 0x1))
                // color = encode(0.0f, 1.0f, 0.0f);
            color = 0xff'00'00'00 | ((tileId & 0x1) ^ !(y & 0x1) ? 0x00'00'ff'00 : 0x00'00'00'ff);
        // }

        // write framebuffer
        args.framebuffer[fb_index++] = color;
    }
    return 0;
}
