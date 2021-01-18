#pragma once

class jObject;
class jCamera;
class jLight;

class jRenderContext
{
public:
	void ResetVisibleArray()
	{
		Visibles.resize(AllObjects.size(), 0);
	}

	std::list<const jLight*> Lights;
	std::vector<const jObject*> AllObjects;
	std::vector<char> Visibles;
	jCamera* Camera = nullptr;
};