#pragma once

class jMeshObject;

class jModelLoader
{
public:
	~jModelLoader();

	static jModelLoader& GetInstance()
	{
		if (!_instance)
			_instance = new jModelLoader();
		return *_instance;
	}

	//bool ConvertToFBX(const char* destFilename, const char* filename) const;

	jMeshObject* LoadFromFile(const char* filename);
	jMeshObject* LoadFromFile(const char* filename, const char* materialRootDir);

private:
	jModelLoader();

	static jModelLoader* _instance;
};

