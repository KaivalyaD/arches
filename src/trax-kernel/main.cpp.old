#include "stdafx.hpp"
#include "include.hpp"
#include "intersect.hpp"
#include "custom-instr.hpp"

inline static uint32_t encode_pixel(rtm::vec3 in)
{
	in = rtm::clamp(in, 0.0f, 1.0f);
	uint32_t out = 0u;
	out |= static_cast<uint32_t>(in.r * 255.0f + 0.5f) << 0;
	out |= static_cast<uint32_t>(in.g * 255.0f + 0.5f) << 8;
	out |= static_cast<uint32_t>(in.b * 255.0f + 0.5f) << 16;
	out |= 0xff << 24;
	return out;
}

inline static void kernel(const TRaXKernelArgs& args)
{
	constexpr uint32_t SPP = 1;
	constexpr uint TILE_X = 4;
	constexpr uint TILE_Y = 8;
	constexpr uint TILE_SIZE = TILE_X * TILE_Y;
	
	for (uint index = fchthrd(); index < args.framebuffer_size; index = fchthrd())
	{
		uint tile_id = index / TILE_SIZE;
		uint32_t tile_x = tile_id % (args.framebuffer_width / TILE_X);
		uint32_t tile_y = tile_id / (args.framebuffer_width / TILE_X);
		uint thread_id = index % TILE_SIZE;
		uint32_t x = tile_x * TILE_X + thread_id % TILE_X;
		uint32_t y = tile_y * TILE_Y + thread_id / TILE_X;
		uint fb_index = y * args.framebuffer_width + x;

		rtm::RNG rng(fb_index);
		IntersectStats stats;

		// path tracing
		float radiance = 0.0f;
		for(uint32_t i = 0; i < SPP; ++i)
		{
			float throughput = 1.0f;
			rtm::Ray ray = args.pregen_rays ? args.rays[fb_index] : args.camera.generate_ray_through_pixel(x, y);
			
			for(uint32_t j = 0; j < 3; ++j)
			{
				rtm::Hit hit(ray.t_max, rtm::vec2(0.0f), ~0u);

				_traceray<0x0u>(index, ray, hit);
				if(hit.t >= ray.t_max)
				{
					radiance += throughput * 2.0f;
					break;
				}
				
				rtm::uvec3 &ni = args.normal_indices[hit.id];
				rtm::vec3 &n1 = args.normals[ni.x];
				rtm::vec3 &n2 = args.normals[ni.y];
				rtm::vec3 &n3 = args.normals[ni.z];
				rtm::vec3 n = hit.bc.x * n1 + hit.bc.y * n2 + (1.0f - hit.bc.x - hit.bc.y) * n3;

				// absorb energy due to bounce
				throughput *= 0.8f;

				// generate secondray rays
				ray.o += ray.d * hit.t;
				ray.d = cosine_sample_hemisphere(n, rng);
			}
		}
		args.framebuffer[fb_index] = encode_pixel(radiance / SPP);
	}
}

#ifdef __riscv 
int main()
{
	kernel(*(const TRaXKernelArgs*)TRAX_KERNEL_ARGS_ADDRESS);
	return 0;
}
#else

int main(int argc, char* argv[])
{
	TRaXKernelArgs args;
	args.framebuffer_width = 512;
	args.framebuffer_height = 512;
	args.framebuffer_size = args.framebuffer_width * args.framebuffer_height;
	std::vector<uint32_t> fb_vec(args.framebuffer_size);
	args.framebuffer = fb_vec.data();

	args.pregen_rays = false;
	args.camera = rtm::Camera(args.framebuffer_width, args.framebuffer_height, 24.0f, rtm::vec3(0.0, 0.0, 5.0), rtm::vec3(0.0f, 0.0f, 0.0f));

	rtm::Sphere spheres[3];
	spheres[0].center = rtm::vec3(-1.5f, -1.0f, 0.0f);
	spheres[0].radius = 2.0f;
	spheres[1].center = rtm::vec3(0.0f, 1.0f, 0.0f);
	spheres[1].radius = 1.0f;
	spheres[2].center = rtm::vec3(1.5f, -1.0f, 0.0f);
	spheres[2].radius = 1.0f;
	args.sphere_list.spheres = spheres;
	args.sphere_list.sphere_count = sizeof(spheres) / sizeof(rtm::Sphere);
	
	std::vector<rtm::Ray> rays(args.framebuffer_size);
	printf("\nStarting Traversal\n");
	auto start = std::chrono::high_resolution_clock::now();

	std::vector<std::thread> threads;
	uint thread_count = 1;
	thread_count = max(std::thread::hardware_concurrency() - 2u, 1u);
	for (uint i = 1; i < thread_count; ++i) threads.emplace_back(kernel, args);
	kernel(args);
	for (uint i = 1; i < thread_count; ++i) threads[i - 1].join();

	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	printf("Runtime: %dms\n", (uint)duration.count());

	stbi_flip_vertically_on_write(true);
	stbi_write_png("out.png", args.framebuffer_width, args.framebuffer_height, 4, args.framebuffer, 0);

	return 0;
}
#endif
