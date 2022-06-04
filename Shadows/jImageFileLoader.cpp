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

std::weak_ptr<jImageData> jImageFileLoader::LoadImageDataFromFile(const jName& filename, bool sRGB)
{
	char Ext[128] = "";
	_splitpath_s(filename.ToStr(), nullptr, 0, nullptr, 0, nullptr, 0, Ext, sizeof(Ext));
	for (int32 i = 0; i < _countof(Ext); ++i)
	{
		if (Ext[i] == '\0')
			break;
		Ext[i] = std::tolower(Ext[i]);
	}
	if (strlen(Ext) <= 0)
		return std::weak_ptr<jImageData>();

	auto it_find = CachedImageDataMap.find(filename);
	if (CachedImageDataMap.end() != it_find)
		return it_find->second;

	const jName ExtName(Ext);
	static jName ExtDDS(".dds");
	static jName ExtPNG(".png");
	static jName ExtHDR(".hdr");

	std::shared_ptr<jImageData> NewImageDataPatr(new jImageData());
	bool IsHDR = false;

	int32 width = 0;
	int32 height = 0;
	int32 NumOfComponent = -1;
	if (ExtName == ExtDDS)
	{
		uint8* imageData = stbi_dds_load(filename.ToStr(), &width, &height, &NumOfComponent, 0);

		int32 NumOfBytes = width * height * sizeof(uint8) * NumOfComponent;
		NewImageDataPatr->ImageData.resize(NumOfBytes);
		memcpy(&NewImageDataPatr->ImageData[0], imageData, NumOfBytes);
		NewImageDataPatr->sRGB = sRGB;
		NewImageDataPatr->FormatType = EFormatType::UNSIGNED_BYTE;

		stbi_image_free(imageData);
	}
	else if (ExtName == ExtPNG)
	{
		NewImageDataPatr->Filename = filename;
		unsigned w, h;
		LodePNG::decode(NewImageDataPatr->ImageData, w, h, filename.ToStr());
		NewImageDataPatr->sRGB = sRGB;
		NewImageDataPatr->FormatType = EFormatType::UNSIGNED_BYTE;

		width = static_cast<int32>(w);
		height = static_cast<int32>(h);
		NumOfComponent = 4;
	}
	else if (ExtName == ExtHDR)
	{
		float* imageData = stbi_loadf(filename.ToStr(), &width, &height, &NumOfComponent, 0);

		int32 NumOfBytes = width * height * sizeof(float) * NumOfComponent;
		NewImageDataPatr->ImageData.resize(NumOfBytes);
		memcpy(&NewImageDataPatr->ImageData[0], imageData, NumOfBytes);
		NewImageDataPatr->sRGB = sRGB;
		NewImageDataPatr->FormatType = EFormatType::FLOAT;

		stbi_image_free(imageData);
		IsHDR = true;
	}
	else
	{
		uint8* imageData = stbi_load(filename.ToStr(), &width, &height, &NumOfComponent, 0);

		int32 NumOfBytes = width * height * sizeof(uint8) * NumOfComponent;
		NewImageDataPatr->ImageData.resize(NumOfBytes);
		memcpy(&NewImageDataPatr->ImageData[0], imageData, NumOfBytes);
		NewImageDataPatr->sRGB = sRGB;
		NewImageDataPatr->FormatType = EFormatType::UNSIGNED_BYTE;

		stbi_image_free(imageData);
	}

	NewImageDataPatr->Width = width;
	NewImageDataPatr->Height = height;

	switch (NumOfComponent)
	{
	case 1:
		NewImageDataPatr->Format = IsHDR ? ETextureFormat::R32F : ETextureFormat::R;
		break;
	case 2:
		NewImageDataPatr->Format = IsHDR ? ETextureFormat::RG32F : ETextureFormat::RG;
		break;
	case 3:
		NewImageDataPatr->Format = IsHDR ? ETextureFormat::RGB32F : ETextureFormat::RGB;
		break;
	case 4:
		NewImageDataPatr->Format = IsHDR ? ETextureFormat::RGBA32F : ETextureFormat::RGBA;
		break;
	default:
		check(0);
		break;
	}

	CachedImageDataMap[filename] = NewImageDataPatr;
	return NewImageDataPatr;
}

std::weak_ptr<jTexture> jImageFileLoader::LoadTextureFromFile(const jName& filename, bool sRGB /*= false*/)
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
