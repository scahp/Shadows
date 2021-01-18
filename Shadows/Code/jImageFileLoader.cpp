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
	int32 w = 0;
	int32 h = 0;
	int32 NumOfComponent = -1;
	if (std::string::npos != filename.find(".tga"))
	{
		uint8* imageData = stbi_load(filename.c_str(), &w, &h, &NumOfComponent, 0);

		int32 NumOfBytes = w * h * sizeof(uint8) * NumOfComponent;
		data.ImageData.resize(NumOfBytes);
		memcpy(&data.ImageData[0], imageData, NumOfBytes);
		data.sRGB = sRGB;

		stbi_image_free(imageData);
	}
	else if (std::string::npos != filename.find(".png"))
	{
		data.Filename = filename;
		unsigned w, h;
		LodePNG::decode(data.ImageData, w, h, filename.c_str());
		data.sRGB = sRGB;

		w = static_cast<int32>(w);
		h = static_cast<int32>(h);
		NumOfComponent = 4;
	}
	else if (std::string::npos != filename.find(".hdr"))
	{
		int w, h, nrComponents;
		float* imageData = stbi_loadf(filename.c_str(), &w, &h, &nrComponents, 0);

		int32 NumOfBytes = w * h * sizeof(float) * nrComponents;
		data.ImageData.resize(NumOfBytes);
		memcpy(&data.ImageData[0], imageData, NumOfBytes);
		data.sRGB = sRGB;
		data.FormatType = EFormatType::FLOAT;

		stbi_image_free(imageData);
	}

	data.Width = w;
	data.Height = h;

	switch (NumOfComponent)
	{
	case 1:
		data.Format = ETextureFormat::R;
		break;
	case 2:
		data.Format = ETextureFormat::RG;
		break;
	case 3:
		data.Format = ETextureFormat::RGB;
		break;
	case 4:
		data.Format = ETextureFormat::RGBA;
		break;
	}
}
