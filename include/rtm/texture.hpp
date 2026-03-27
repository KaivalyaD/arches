#pragma once
#include "vec3.hpp"

#ifndef __riscv
#include "stbi/stb_image.h"
#include "stbi/stb_image_write.h"
#include <vector>
#endif

class Texture2D
{
public:
	union Texel
	{
		uint8_t channel[4];
		uint32_t raw;
	};

	int32_t width;
	int32_t height;
	int32_t comp;
	Texel* texels;

public:
	Texture2D() : texels(nullptr) {};
#ifndef __riscv
	Texture2D(std::string filename)
	{
		printf("Loading: %s\r", filename.c_str());
		stbi_set_flip_vertically_on_load(true);

		stbi_uc* data = stbi_load(filename.c_str(), &width, &height, &comp, 0);
		if(data)
		{
			uint32_t size = width * height;
			texels = (Texel*)malloc(sizeof(Texel) * size);
			for(uint32_t j = 0; j < height; ++j)
				for(uint32_t i = 0; i < width; ++i)
					for(uint32_t k = 0; k < comp; ++k)
						get_texel_addr(rtm::uvec2(i, j))->channel[k] = data[(j * width + i) * comp + k];

			stbi_image_free(data);
			printf("Loaded: %s \n", filename.c_str());
		}
		else
		{
			texels = nullptr;
			printf("Failed: %s \n", filename.c_str());
		}
	}

	Texture2D(const Texture2D& other)
	{
		memcpy(this, &other, sizeof(Texture2D));
		uint32_t size = sizeof(Texel) * width * height;
		texels = (Texel*)malloc(size);
		memcpy(texels, other.texels, size);
	}

	Texture2D& operator=(const Texture2D& other)
	{
		if(texels) free(texels);
		memcpy(this, &other, sizeof(Texture2D));
		uint32_t size = sizeof(Texel) * width * height;
		texels = (Texel*)malloc(size);
		memcpy(texels, other.texels, size);
		return *this;
	}

	~Texture2D()
	{
		if(texels) 
			free(texels);
	}
#endif

	rtm::vec4 sample(const rtm::vec2& uv) const
	{
		if(width == 0 && height == 0) return rtm::vec4(0.0f);
		return read_nearest(uv);
	}

	rtm::uvec2 get_iuv(const rtm::vec2& uv, const rtm::vec2& offset = rtm::vec2(0.0f)) const
	{
		rtm::vec2 fuv = uv * rtm::vec2(width, height) + offset;
		return rtm::uvec2(fuv[0], fuv[1]);
	}

	rtm::vec2 get_fract_uv(const rtm::vec2& uv) const
	{
		rtm::vec2 fuv = uv * rtm::vec2(width, height);
		return (fuv - rtm::vec2((int32_t)fuv[0], (int32_t)fuv[1]));
	}

	Texel* get_texel_addr(const rtm::uvec2& iuv) const
	{
		uint32_t x = iuv[0] % width;
		uint32_t y = iuv[1] % height;
	#if 0
		return &texels[y * width + x];
	#else
		static const uint32_t B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF};
		static const uint32_t S[] = {1, 2, 4, 8};

		x = (x | (x << S[3])) & B[3];
		x = (x | (x << S[2])) & B[2];
		x = (x | (x << S[1])) & B[1];
		x = (x | (x << S[0])) & B[0];

		y = (y | (y << S[3])) & B[3];
		y = (y | (y << S[2])) & B[2];
		y = (y | (y << S[1])) & B[1];
		y = (y | (y << S[0])) & B[0];

		return &texels[x | (y << 1)];
	#endif
	}

	static rtm::vec4 decode_texel(const Texel& texel)
	{
		return rtm::vec4(texel.channel[0], texel.channel[1], texel.channel[2], texel.channel[3]) * (1.0f / 255.0f);
	}

	rtm::vec4 read_nearest(const rtm::vec2& uv, rtm::vec2 offset = rtm::vec2(0.0f)) const
	{
		rtm::uvec2 iuv = get_iuv(uv, offset);
		Texel* texel = get_texel_addr(iuv);
		return decode_texel(*texel);
	}
};

