#include "pch.h"
#include "jImageFileLoader.h"
#include "lodepng.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

jImageFileLoader* jImageFileLoader::_instance = nullptr;

jImageFileLoader::jImageFileLoader()
{
}


jImageFileLoader::~jImageFileLoader()
{
}

void jImageFileLoader::LoadTextureFromFile(jImageData& data, std::string const& filename, bool sRGB)
{
	if (std::string::npos != filename.find(".tga"))
	{
		// todo 
	}
	else if (std::string::npos != filename.find(".png"))
	{
		data.Filename = filename;
		unsigned w, h;
		LodePNG::decode(data.ImageData, w, h, filename.c_str());
		data.Width = static_cast<int32>(w);
		data.Height = static_cast<int32>(h);
		data.sRGB = sRGB;
	}
	else if (std::string::npos != filename.find(".hdr"))
	{
		int w, h, nrComponents;
		float* imageData = stbi_loadf(filename.c_str(), &w, &h, &nrComponents, 0);

		int32 NumOfBytes = w * h * sizeof(float) * nrComponents;
		data.ImageData.resize(NumOfBytes);
		memcpy(&data.ImageData[0], imageData, NumOfBytes);
		data.Width = w;
		data.Height = h;
		data.sRGB = sRGB;

		stbi_image_free(imageData);
	}
	else
	{
		int w, h, nrComponents;
		uint8* imageData = stbi_load(filename.c_str(), &w, &h, &nrComponents, 0);

		int32 NumOfBytes = w * h * sizeof(uint8) * nrComponents;
		data.ImageData.resize(NumOfBytes);
		memcpy(&data.ImageData[0], imageData, NumOfBytes);
		data.Width = w;
		data.Height = h;
		data.sRGB = sRGB;

		stbi_image_free(imageData);
	}
}
