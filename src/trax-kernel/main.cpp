#include "stdafx.hpp"
#include "include.hpp"
#include "custom-instr.hpp"

int main(void)
{
    // framebuffer address format:
    // suppose framebuffer size is 256 x 256. Then, we get a 1-D index into framebuffer as:
    //    u32 index  = 0xYYXX
    // subdividing index, we can get 2-D image-space coords:
    //    u32 frame_x = 0xXX
    //    u32 frame_y = 0xYY
    // subdividing image-space coords, we can construct tiles with coords:
    //    u32 tile_x  = frame_x >> 2 // where 1 << 2 = 4 is the tile_width
    //    u32 tile_y  = frame_y >> 3 // where 1 << 3 = 8 is the tile_height
    // using tile and image-space coords, we can obtain  local to the tile:
    //    u32 tile_local_x = (tile_x * tile_width)  + (frame_x & 0b'0011) // 0b'0011 = ~(~0 << 2)
    //    u32 tile_local_y = (tile_y * tile_height) + (frame_y & 0b'0111) // 0b'0111 = ~(~0 << 3)
    constexpr uint TILE_X_INDEX_BITS  = 2;
    constexpr uint TILE_Y_INDEX_BITS  = 3;
    constexpr uint TILE_WIDTH         = 1 << TILE_X_INDEX_BITS;
    constexpr uint TILE_HEIGHT        = 1 << TILE_Y_INDEX_BITS;
    constexpr uint TILE_X_OFFSET_MASK = ~(~0U << TILE_X_INDEX_BITS);
    constexpr uint TILE_Y_OFFSET_MASK = ~(~0U << TILE_Y_INDEX_BITS);

    const TRaXKernelArgs args = *(TRaXKernelArgs *)(TRAX_KERNEL_ARGS_ADDRESS);
    for (uint index = fchthrd(); index < args.framebuffer_size; index = fchthrd())
	{
        uint frame_x   = index   %  args.framebuffer_width;
        uint frame_y   = index   /  args.framebuffer_width;
        uint tile_x    = frame_x >> TILE_X_INDEX_BITS;
        uint tile_y    = frame_y >> TILE_Y_INDEX_BITS;
        uint x         = tile_x  *  TILE_WIDTH  + (frame_x & TILE_X_OFFSET_MASK);
        uint y         = tile_y  *  TILE_HEIGHT + (frame_y & TILE_Y_OFFSET_MASK);
        uint fb_index  = x * args.framebuffer_width + y;

        if((tile_x & 0x1) ^ (tile_y & 0x1))
            args.framebuffer[fb_index] = 0xff'00'f0'0f;
        else
            args.framebuffer[fb_index] = 0xff'00'0f'f0;
    }
    return 0;
}
