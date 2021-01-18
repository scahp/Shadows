#pragma once

class jHairObject;

class jHairModelLoader
{
public:
	~jHairModelLoader();

	static jHairModelLoader& GetInstance()
	{
		if (!_instance)
			_instance = new jHairModelLoader();
		return *_instance;
	}

	jHairObject* CreateHairObject(const char* filename);

private:
	jHairModelLoader();
	static jHairModelLoader* _instance;
};

