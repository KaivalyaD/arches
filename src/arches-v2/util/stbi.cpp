//stb image
#include "stdafx.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef BUILD_PLATFORM_WINDOWS
    #define STBI_MSC_SECURE_CRT
#endif

//stb image
#include "stb_image.h"
#include "stb_image_write.h"