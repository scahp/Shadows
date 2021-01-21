#include "pch.h"
#include "jImageFileLoader.h"
#include "lodepng.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "SOIL_STBI/stbi_DDS_aug_c.h"

jImageFileLoader* jImageFileLoader::_instance = nullptr;

jImageFileLoader::jImageFileLoader()
{
}


jImageFileLoader::~jImageFileLoader()
{
}

std::weak_ptr<jImageData> jImageFileLoader::LoadImageDataFromFile(std::string const& filename, bool sRGB)
{
	auto it_find = CachedImageDataMap.find(filename);
	if (CachedImageDataMap.end() != it_find)
		return it_find->second;

	std::shared_ptr<jImageData> NewImageDataPatr(new jImageData());

	int32 width = 0;
	int32 height = 0;
	int32 NumOfComponent = -1;
	if (std::string::npos != filename.find(".tga"))
	{
		uint8* imageData = stbi_load(filename.c_str(), &width, &height, &NumOfComponent, 0);

		int32 NumOfBytes = width * height * sizeof(uint8) * NumOfComponent;
		NewImageDataPatr->ImageData.resize(NumOfBytes);
		memcpy(&NewImageDataPatr->ImageData[0], imageData, NumOfBytes);
		NewImageDataPatr->sRGB = sRGB;
		NewImageDataPatr->FormatType = EFormatType::UNSIGNED_BYTE;

		stbi_image_free(imageData);
	}
	else if (std::string::npos != filename.find(".dds"))
	{
		uint8* imageData = stbi_dds_load(filename.c_str(), &width, &height, &NumOfComponent, 0);

		int32 NumOfBytes = width * height * sizeof(uint8) * NumOfComponent;
		NewImageDataPatr->ImageData.resize(NumOfBytes);
		memcpy(&NewImageDataPatr->ImageData[0], imageData, NumOfBytes);
		NewImageDataPatr->sRGB = sRGB;
		NewImageDataPatr->FormatType = EFormatType::UNSIGNED_BYTE;

		stbi_image_free(imageData);
	}
	else if (std::string::npos != filename.find(".png"))
	{
		NewImageDataPatr->Filename = filename;
		unsigned w, h;
		LodePNG::decode(NewImageDataPatr->ImageData, w, h, filename.c_str());
		NewImageDataPatr->sRGB = sRGB;
		NewImageDataPatr->FormatType = EFormatType::UNSIGNED_BYTE;

		width = static_cast<int32>(w);
		height = static_cast<int32>(h);
		NumOfComponent = 4;
	}
	else if (std::string::npos != filename.find(".hdr"))
	{
		int w, h, nrComponents;
		float* imageData = stbi_loadf(filename.c_str(), &w, &h, &nrComponents, 0);

		int32 NumOfBytes = w * h * sizeof(float) * nrComponents;
		NewImageDataPatr->ImageData.resize(NumOfBytes);
		memcpy(&NewImageDataPatr->ImageData[0], imageData, NumOfBytes);
		NewImageDataPatr->sRGB = sRGB;
		NewImageDataPatr->FormatType = EFormatType::FLOAT;

		stbi_image_free(imageData);
	}

	NewImageDataPatr->Width = width;
	NewImageDataPatr->Height = height;

	switch (NumOfComponent)
	{
	case 1:
		NewImageDataPatr->Format = ETextureFormat::R;
		break;
	case 2:
		NewImageDataPatr->Format = ETextureFormat::RG;
		break;
	case 3:
		NewImageDataPatr->Format = ETextureFormat::RGB;
		break;
	case 4:
		NewImageDataPatr->Format = ETextureFormat::RGBA;
		break;
	}

	CachedImageDataMap[filename] = NewImageDataPatr;
	return NewImageDataPatr;
}

std::weak_ptr<jTexture> jImageFileLoader::LoadTextureFromFile(std::string const& filename, bool sRGB /*= false*/)
{
	auto it_find = CachedTextureMap.find(filename);
	if (CachedTextureMap.end() != it_find)
		return it_find->second;

	std::shared_ptr<jTexture> NewTexture;

	std::weak_ptr<jImageData> imageDataWeakPtr = LoadImageDataFromFile(filename, sRGB);
	jImageData* pImageData = imageDataWeakPtr.lock().get();
	JASSERT(pImageData);
	if (pImageData)
	{
		jTexture* pCreatedTexture = g_rhi->CreateTextureFromData(&pImageData->ImageData[0], pImageData->Width
			, pImageData->Height, pImageData->sRGB, pImageData->FormatType, pImageData->Format, true);
		NewTexture = std::shared_ptr<jTexture>(pCreatedTexture);
	}

	CachedTextureMap[filename] = NewTexture;
	return NewTexture;
}
