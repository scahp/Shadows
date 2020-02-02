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
	//g_rhi->EnableDepthTest(true);
	//auto renderTarget = Game.DirectionalLight->GetShadowMapRenderTarget();
	//if (renderTarget->Begin())
	//{
	//	auto texture_gl = static_cast<jTexture_OpenGL*>(Game.DirectionalLight->GetShadowMapRenderTarget()->GetTextureDepth());
	//	auto hiz_gen = jShader::GetShader("Hi-Z_Gen");
	//	auto hiz_gen_gl = static_cast<jShader_OpenGL*>(hiz_gen);

	//	int32 drawCallCount = 0;
	//	int32 TempWidth = 512;
	//	int32 TempHeight = 512;

	//	glUseProgram(hiz_gen_gl->program);

	//	// disable color buffer as we will render only a depth image
	//	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	//	glUniform1i(glGetUniformLocation(hiz_gen_gl->program, "LastMip"), 0);
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, texture_gl->TextureID);
	//	g_rhi->BindSamplerState(0, Game.DirectionalLight->ShadowMapData->ShadowMapSamplerState.get());
	//	// we have to disable depth testing but allow depth writes
	//	glDepthFunc(GL_ALWAYS);
	//	// calculate the number of mipmap levels for NPOT texture
	//	int numLevels = 1 + (int)floorf(log2f(fmaxf(TempWidth, TempHeight)));
	//	int currentWidth = TempWidth;
	//	int currentHeight = TempHeight;
	//	for (int i = 1; i < numLevels; i++) {
	//		glUniform2i(glGetUniformLocation(hiz_gen_gl->program, "LastMipSize"), currentWidth, currentHeight);
	//		// calculate next viewport size
	//		currentWidth /= 2;
	//		currentHeight /= 2;
	//		// ensure that the viewport size is always at least 1x1
	//		currentWidth = currentWidth > 0 ? currentWidth : 1;
	//		currentHeight = currentHeight > 0 ? currentHeight : 1;
	//		glViewport(0, 0, currentWidth, currentHeight);
	//		// bind next level for rendering but first restrict fetches only to previous level
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, i - 1);
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i - 1);
	//		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_gl->TextureID, i);
	//		// dummy draw command as the full screen quad is generated completely by a geometry shader
	//		glDrawArrays(GL_POINTS, 0, 1);
	//		drawCallCount++;
	//	}
	//	// reset mipmap level range for the depth image
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
	//	
	//	// reset the framebuffer configuration
	//	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_gl->TextureID, 0);
	//	
	//	// reenable color buffer writes, reset viewport and reenable depth test
	//	glDepthFunc(GL_LEQUAL);
	//	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	//	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	//	g_rhi->BindSamplerState(0, nullptr);
	//}
	//renderTarget->End();

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
