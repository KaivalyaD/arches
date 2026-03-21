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

private:
	// Used by Arches to shallow copy textures
	// Not to be used by kernels!!!
	uint32_t *pTexRefCount;

public:
	Texture2D() : texels(nullptr), pTexRefCount(nullptr) {};
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
			for(uint32_t i = 0; i < size; ++i)
				for(uint32_t j = 0; j < comp; ++j)
					texels[i].channel[j] = data[i * comp + j];

			stbi_image_free(data);

			pTexRefCount = (uint32_t *)malloc(sizeof(uint32_t));
			*pTexRefCount = 1;  // texture acquired first time
			
			printf("Loaded: %s \n", filename.c_str());
		}
		else
		{
			texels = nullptr;
			pTexRefCount = nullptr;
			printf("Failed: %s \n", filename.c_str());
		}
	}

	// Texture2D(const Texture2D& other)
	// {
	// 	memcpy(this, &other, sizeof(Texture2D));
	// 	uint32_t size = sizeof(Texel) * width * height;
	// 	texels = (Texel*)malloc(size);
	// 	memcpy(texels, other.texels, size);
	// }
	Texture2D(const Texture2D& other) : width(other.width), height(other.height), comp(other.comp), pTexRefCount(other.pTexRefCount)
    {
		texels = other.texels;
		if(pTexRefCount)
			acquire_texture_resource();
    }

	// Texture2D& operator=(const Texture2D& other)
	// {
	// 	if(texels) free(texels);
	// 	memcpy(this, &other, sizeof(Texture2D));
	// 	uint32_t size = sizeof(Texel) * width * height;
	// 	texels = (Texel*)malloc(size);
	// 	memcpy(texels, other.texels, size);
	// 	return *this;
	// }
	Texture2D& operator=(const Texture2D& other) 
    {
        if (this == &other)
			return *this;

		// acquire ownership of other's resource
		// this is safer than lose-first-acquire-next
		if(other.pTexRefCount)
			++(*other.pTexRefCount);

		release_texture_resource();  // lose previous ownership
		
		// assign other to this
		width = other.width;
        height = other.height;
        comp = other.comp;
		texels = other.texels;
		pTexRefCount = other.pTexRefCount;
        return *this;
    }

	~Texture2D()
	{
		release_texture_resource();
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

	Texel* get_texel_addr(const rtm::uvec2& iuv) const
	{
		uint32_t x = iuv[0] % width;
		uint32_t y = iuv[1] % height;
		return &texels[y * width + x];
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

#ifndef __riscv
private:
	void acquire_texture_resource(void)
	{
		if (!pTexRefCount)
			return;
		
		(*pTexRefCount)++;
	}
	
	void release_texture_resource(void)
	{
		if (!pTexRefCount)
			return;
		
		(*pTexRefCount)--;

		if((*pTexRefCount) <= 0)
		{
			if(texels)
				free(texels);
			
			free(pTexRefCount);
			texels = nullptr;
			pTexRefCount = nullptr;
		}
	}
#endif
};

