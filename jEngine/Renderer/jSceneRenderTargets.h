﻿#pragma once

struct jRenderTarget;
class jSwapchainImage;

struct jSceneRenderTarget
{
    std::shared_ptr<jRenderTarget> ColorPtr;
    std::shared_ptr<jRenderTarget> DepthPtr;
    std::shared_ptr<jRenderTarget> ResolvePtr;
    std::shared_ptr<jRenderTarget> DirectionalLightShadowMapPtr;

    std::shared_ptr<jRenderTarget> FinalColorPtr;

    void Create(const jSwapchainImage* image);
    void Return();
    bool IsValid() const;
};