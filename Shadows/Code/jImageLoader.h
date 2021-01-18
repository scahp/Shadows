#pragma once
class jImageLoader
{
public:
	jImageLoader();
	~jImageLoader();

	void LoadJpg(std::vector<char>& out, const char* filename);
	void LoadPng(std::vector<char>& out, const char* filename);
	void Load(std::vector<char>& out, const char* filename);
};

