#include "pch.h"
#include "jEngine.h"
#include "jAppSettings.h"
#include "jShadowAppProperties.h"
#include "jPipeline.h"
#include "jShader.h"
#include "jPerformanceProfile.h"

#include "jSamplerStatePool.h"
#include "jRHI_OpenGL.h"
#include "jLight.h"
#include "jRenderTargetPool.h"
#include "jCamera.h"
#include "jRenderObject.h"
#include "jGame.h"
#include "jObject.h"
#include "jForwardRenderer.h"

jEngine::jEngine()
{
}


jEngine::~jEngine()
{
	Game.Teardown();

	jShadowAppSettingProperties::GetInstance().Teardown(jAppSettings::GetInstance().Get("MainPannel"));
}

void jEngine::Init()
{
	jSamplerStatePool::CreateDefaultSamplerState();
	jShaderInfo::CreateShaders();
	IPipeline::SetupPipelines();

	g_rhi->EnableSRGB(false);

	Game.Setup();

	jShadowAppSettingProperties::GetInstance().Setup(jAppSettings::GetInstance().Get("MainPannel"));
}

void jEngine::ProcessInput()
{
	Game.ProcessInput();
}

void jEngine::Update(float deltaTime)
{
	SCOPE_PROFILE(Engine_Update);
	SCOPE_GPU_PROFILE(Engine_Update);

	g_timeDeltaSecond = deltaTime;
	Game.Update(deltaTime);

	// #hizgen
	g_rhi->EnableDepthTest(true);
	auto renderTarget = static_cast<jForwardRenderer*>(Game.ForwardRenderer)->RenderTarget;
	if (renderTarget->Begin())
	{
		auto depthTexture = renderTarget->GetTextureDepth();
		auto hiz_gen = jShader::GetShader("Hi-Z_Gen");

		int32 drawCallCount = 0;
		float TempWidth = static_cast<float>(SCR_WIDTH);
		float TempHeight = static_cast<float>(SCR_HEIGHT);

		g_rhi->SetShader(hiz_gen);

		// Color buffer를 모두 끄고, depth buffer에만 렌더링
		g_rhi->SetColorMask(false, false, false, false);

		// 소스로 사용할 Depth texture 를 설정해준다.
		//  - depthTexture가 소스로도 현재 framebuffer의 depthbuffer 로도 쓰임. (한 쉐이더에서 동시 읽고 쓰고, 다른 mipmap-level 이면 괜찮음)
		SET_UNIFORM_BUFFER_STATIC(int32, "LastMip", 0, hiz_gen);
		g_rhi->SetTexture(0, depthTexture);

		g_rhi->BindSamplerState(0, jSamplerStatePool::GetSamplerState("LinearClampMipmap").get());
		
		// Depth test는 끄고, depth write 쓰는도록 허용
		g_rhi->SetDepthFunc(EComparisonFunc::ALWAYS);

		// NPOT(Not power of two) Texture의 mipmap-level 개수를 계산한다.
		int32 numLevels = 1 + (int)floorf(log2f(fmaxf(TempWidth, TempHeight)));
		int32 currentWidth = static_cast<int32>(TempWidth);
		int32 currentHeight = static_cast<int32>(TempHeight);
		for (int32 i = 1; i < numLevels; i++) 
		{
			// 마지막으로 렌더링한 뷰포트 사이즈 Uniform 변수로 전달
			SET_UNIFORM_BUFFER_STATIC(Vector2i, "LastMipSize", Vector2i(currentWidth, currentHeight), hiz_gen);

			// 다음 뷰포트 사이즈 계산
			currentWidth /= 2;
			currentHeight /= 2;

			// 뷰포트 사이즈가 적어도 1x1 이 되도록 보장
			currentWidth = currentWidth > 0 ? currentWidth : 1;
			currentHeight = currentHeight > 0 ? currentHeight : 1;

			g_rhi->SetViewport(0, 0, currentWidth, currentHeight);

			// 이번에 소스로 사용할 mipmap-level 을 설정해줌. (for문의 인덱스가 1부터 시작함을 참고)
			g_rhi->SetTextureMipmapLevelLimit(ETextureType::TEXTURE_2D, i - 1, i - 1);

			renderTarget->SetDepthTexture(depthTexture, i);

			// 지오메트리 쉐이더에서 Fullscreen Quad를 만들어줄 것이므로 여기서는 1개의 더미 버택스를 그림.
			g_rhi->DrawArrays(EPrimitiveType::POINTS, 0, 1);
			drawCallCount++;
		}
		
		// Depth texture의 mipmap-level 범위 복원
		g_rhi->SetTextureMipmapLevelLimit(ETextureType::TEXTURE_2D, 0, numLevels - 1);
		
		// Framebuffer의 depth buffer의 mipmap-level을 0으로 복원
		renderTarget->SetDepthTexture(depthTexture, 0);
		
		// 컬러버퍼 쓰기가능 상태로 되돌리고, ViewportSize와 Depth func 설정 되돌림
		g_rhi->SetDepthFunc(EComparisonFunc::LESS);
		g_rhi->SetColorMask(true, true, true, true);
		g_rhi->SetViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		g_rhi->BindSamplerState(0, nullptr);
	}
	renderTarget->End();

	static bool test = false;
	static jQueryPrimitiveGenerated* QueryPrimitiveGenerated = g_rhi->CreateQueryPrimitiveGenerated();
	static ITransformFeedbackBuffer* TransformFeedback = g_rhi->CreateTransformFeedbackBuffer("TransformFeedback0");
	if (!test)
	{
		test = true;

		auto hiZOcclusionCull = jShader::GetShader("HiZOcclusionCull");
		auto hiZOcclusionCull_gl = static_cast<jShader_OpenGL*>(hiZOcclusionCull);
		
		TransformFeedback->Init();
		TransformFeedback->UpdateBufferData(nullptr, 400);
		TransformFeedback->UpdateVaryingsToShader({ "OutVisible" }, hiZOcclusionCull);		
	}

	glDisable(GL_MULTISAMPLE);
	static auto s_renderTarget = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget(
		{ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH, SCR_HEIGHT, 1 }));
	uint64 Result = 0;
	if (s_renderTarget->Begin())
	{
		g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });

		auto texture_gl = static_cast<jTexture_OpenGL*>(static_cast<jForwardRenderer*>(Game.ForwardRenderer)->RenderTarget->GetTextureDepth());
		static float scale = 1.2f;
		static bool test = true;
		if (test)
		{
			auto hiZOcclusionCull = jShader::GetShader("HiZOcclusionCull");
			if (hiZOcclusionCull)
			{
				g_rhi->SetShader(hiZOcclusionCull);

				SET_UNIFORM_BUFFER_STATIC(Vector4, "ViewportSize", Vector4(0.0f, 0.0f, SCR_WIDTH, SCR_HEIGHT), hiZOcclusionCull);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "Color", Vector4(1.0f, 0.0f, 0.0f, 0.0f), hiZOcclusionCull);
				SET_UNIFORM_BUFFER_STATIC(int32, "HiZBuffer", 0, hiZOcclusionCull);
				g_rhi->SetTexture(0, texture_gl);

				TransformFeedback->Bind(hiZOcclusionCull);
				TransformFeedback->Begin(EPrimitiveType::POINTS);
				
				g_rhi->EnableRasterizerDiscard(true);
				QueryPrimitiveGenerated->Begin();

				auto BoundBox = Game.Cube->RenderObject->BoundBox;
				Game.Cube->RenderObject->BoundBox.Max *= scale;
				Game.Cube->RenderObject->BoundBox.Min *= scale;
				Game.Cube->RenderObject->DrawBoundBox(Game.MainCamera, hiZOcclusionCull);
				Game.Cube->RenderObject->BoundBox = BoundBox;
				QueryPrimitiveGenerated->End();
				TransformFeedback->End();

				float feedback[100];
				TransformFeedback->GetBufferData(feedback, sizeof(feedback));

				Result = QueryPrimitiveGenerated->GetResult();
				g_rhi->GetQueryPrimitiveGeneratedResult(QueryPrimitiveGenerated);
				g_rhi->EnableRasterizerDiscard(false);
			}
		}

		if (!test)
		{
			QueryPrimitiveGenerated->Begin();
			auto boundingBox = jShader::GetShader("BoundingBox");
			if (boundingBox)
			{
				g_rhi->SetShader(boundingBox);
				auto BoundBox = Game.Cube->RenderObject->BoundBox;
				Game.Cube->RenderObject->BoundBox.Max *= scale;
				Game.Cube->RenderObject->BoundBox.Min *= scale;
				Game.Cube->RenderObject->DrawBoundBox(Game.MainCamera, boundingBox);
				Game.Cube->RenderObject->BoundBox = BoundBox;
			}
			QueryPrimitiveGenerated->End();
			Result = QueryPrimitiveGenerated->GetResult();
		}

		s_renderTarget->End();
	}

	jShader::UpdateShaders();
}

void jEngine::Resize(int width, int height)
{
	glViewport(0, 0, width, height);
}

void jEngine::OnMouseMove(int32 xOffset, int32 yOffset)
{
	Game.OnMouseMove(xOffset, yOffset);
}
