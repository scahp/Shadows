#pragma once

#include "jGame.h"

class jEngine
{
public:
	jEngine();
	~jEngine();

	void Init();
	void ProcessInput();
	void Update(float deltaTime);
	void Resize(int width, int height);
	void OnMouseMove(int32 xOffset, int32 yOffset);

	jGame Game;

	class jAppSettingProperties* AppSettingProperties = nullptr;
};

