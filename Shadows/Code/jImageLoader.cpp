#include "pch.h"
#include "jImageLoader.h"


jImageLoader::jImageLoader()
{
}


jImageLoader::~jImageLoader()
{
}

void jImageLoader::LoadJpg(std::vector<char>& out, const char* filename)
{

}

void jImageLoader::LoadPng(std::vector<char>& out, const char* filename)
{

}

void jImageLoader::Load(std::vector<char>& out, const char* filename)
{
	const auto length = strlen(filename);
	if (length <= 4)
		return;

	if (strstr(&filename[length - 4], ".jpg"))
	{
		LoadJpg(out, filename);
	}
	else if (strstr(&filename[length - 4], ".png"))
	{
		LoadPng(out, filename);
	}
}
