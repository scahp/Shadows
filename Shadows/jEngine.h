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
	void OnMouseInput(EMouseButtonType buttonType, Vector2 pos, bool buttonDown, bool isChanged);

	jGame Game;

	class jAppSettingProperties* AppSettingProperties = nullptr;
};

