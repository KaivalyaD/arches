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
    constexpr uint TILE_WIDTH  = 4;
    constexpr uint TILE_HEIGHT = 8;
    constexpr uint TILE_SIZE   = TILE_WIDTH * TILE_HEIGHT;

    const TRaXKernelArgs args = *(TRaXKernelArgs *)(TRAX_KERNEL_ARGS_ADDRESS);
    for (uint index = fchthrd(); index < args.framebuffer_size; index = fchthrd())
	{
        uint thread_id = index % TILE_SIZE;
        uint tileIndex = index / TILE_SIZE;
        uint tileX     = tileIndex % (args.framebuffer_width / TILE_WIDTH);
        uint tileY     = tileIndex / (args.framebuffer_width / TILE_WIDTH);
		uint x         = tileX * TILE_WIDTH + thread_id % TILE_WIDTH;
		uint y         = tileY * TILE_HEIGHT + thread_id / TILE_WIDTH;
		uint fb_index = y * args.framebuffer_width + x;

        // trace a primary ray for each pixel within this tile
        // generate
        rtm::Ray ray = args.camera.generate_ray_through_pixel(x, y);
        
        // trace
        rtm::Hit hit(ray.t_max, rtm::vec2(0.0f), ~0U);
        _traceray<IGNORE>(IGNORE, ray, hit);

        // shade
        uint color = encode(0.3f, 0.3f, 0.3f); // miss
        if(hit.t < ray.t_max)
        {
            color = encode(1.0f, 0.0f, 0.0f); // hit
        }

        // write framebuffer
        args.framebuffer[fb_index++] = color;
    }
    return 0;
}
