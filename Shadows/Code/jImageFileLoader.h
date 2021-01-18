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

	static jImageFileLoader& GetInstance() { return *_instance; }

	void LoadTextureFromFile(jImageData& data, std::string const& filename, bool sRGB = false);

private:
	jImageFileLoader();

	static jImageFileLoader* _instance;
};

