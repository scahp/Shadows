#pragma once

struct jImageData
{
	bool sRGB = false;
	int32 Width = 0;
	int32 Height = 0;
	std::string Filename;
	std::vector<unsigned char> ImageData;
	ETextureFormat Format = ETextureFormat::RGBA;
	EFormatType FormatType = EFormatType::UNSIGNED_BYTE;
};

class jImageFileLoader
{
public:
	~jImageFileLoader();

	static jImageFileLoader& GetInstance()
	{
		if (!_instance)
			_instance = new jImageFileLoader();
		return *_instance; 
	}

	std::weak_ptr<jImageData> LoadImageDataFromFile(std::string const& filename, bool sRGB = false);
	std::weak_ptr<jTexture> LoadTextureFromFile(std::string const& filename, bool sRGB = false);

private:
	std::map<std::string, std::shared_ptr<jImageData> > CachedImageDataMap;
	std::map<std::string, std::shared_ptr<jTexture> > CachedTextureMap;

private:
	jImageFileLoader();

	static jImageFileLoader* _instance;
};

