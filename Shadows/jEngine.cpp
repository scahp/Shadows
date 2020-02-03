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
		auto texture_gl = static_cast<jTexture_OpenGL*>(renderTarget->GetTextureDepth());
		auto hiz_gen = jShader::GetShader("Hi-Z_Gen");
		auto hiz_gen_gl = static_cast<jShader_OpenGL*>(hiz_gen);

		int32 drawCallCount = 0;
		int32 TempWidth = SCR_WIDTH;
		int32 TempHeight = SCR_HEIGHT;

		glUseProgram(hiz_gen_gl->program);

		// disable color buffer as we will render only a depth image
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glUniform1i(glGetUniformLocation(hiz_gen_gl->program, "LastMip"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_gl->TextureID);
		g_rhi->BindSamplerState(0, jSamplerStatePool::GetSamplerState("LinearClampMipmap").get());
		// we have to disable depth testing but allow depth writes
		glDepthFunc(GL_ALWAYS);
		// calculate the number of mipmap levels for NPOT texture
		int numLevels = 1 + (int)floorf(log2f(fmaxf(TempWidth, TempHeight)));
		int currentWidth = TempWidth;
		int currentHeight = TempHeight;
		for (int i = 1; i < numLevels; i++) {
			glUniform2i(glGetUniformLocation(hiz_gen_gl->program, "LastMipSize"), currentWidth, currentHeight);
			// calculate next viewport size
			currentWidth /= 2;
			currentHeight /= 2;
			// ensure that the viewport size is always at least 1x1
			currentWidth = currentWidth > 0 ? currentWidth : 1;
			currentHeight = currentHeight > 0 ? currentHeight : 1;
			glViewport(0, 0, currentWidth, currentHeight);
			// bind next level for rendering but first restrict fetches only to previous level
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, i - 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i - 1);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_gl->TextureID, i);
			// dummy draw command as the full screen quad is generated completely by a geometry shader
			glDrawArrays(GL_POINTS, 0, 1);
			drawCallCount++;
		}
		// reset mipmap level range for the depth image
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
		
		// reset the framebuffer configuration
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_gl->TextureID, 0);
		
		// reenable color buffer writes, reset viewport and reenable depth test
		glDepthFunc(GL_LESS);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		g_rhi->BindSamplerState(0, nullptr);
	}
	renderTarget->End();

	static uint32 CullQuery = 0;
	static uint32 TFBO = 0;
	static bool test = false;
	if (!test)
	{
		test = true;
		glGenQueries(1, &CullQuery);
		glGenBuffers(1, &TFBO);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, TFBO);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(float)*100, nullptr, GL_DYNAMIC_COPY);

		auto hiZOcclusionCull = jShader::GetShader("HiZOcclusionCull");
		auto hiZOcclusionCull_gl = static_cast<jShader_OpenGL*>(hiZOcclusionCull);

		
		//const char* vars[] = { "Test1", "Test2", "Test3", "Test4" };
		//glTransformFeedbackVaryings(hiZOcclusionCull_gl->program, 4, vars, GL_INTERLEAVED_ATTRIBS);

		const char* vars[] = { "OutVisible" };
		glTransformFeedbackVaryings(hiZOcclusionCull_gl->program, 1, vars, GL_INTERLEAVED_ATTRIBS);

		
		glLinkProgram(hiZOcclusionCull_gl->program);
		int32 isValid = 0;
		glGetProgramiv(hiZOcclusionCull_gl->program, GL_LINK_STATUS, &isValid);
		if (!isValid)
		{
			int maxLength = 0;
			glGetProgramiv(hiZOcclusionCull_gl->program, GL_INFO_LOG_LENGTH, &maxLength);

			if (maxLength > 0)
			{
				std::vector<char> errorLog(maxLength + 1, 0);
				glGetProgramInfoLog(hiZOcclusionCull_gl->program, maxLength, &maxLength, &errorLog[0]);
				JMESSAGE(&errorLog[0]);
			}
		}
		
	}

	glDisable(GL_MULTISAMPLE);
	static auto s_renderTarget = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget(
		{ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH, SCR_HEIGHT, 1 }));
	int32 Result = 0;
	if (s_renderTarget->Begin())
	{
		g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });

		auto texture_gl = static_cast<jTexture_OpenGL*>(static_cast<jForwardRenderer*>(Game.ForwardRenderer)->RenderTarget->GetTextureDepth());
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_gl->TextureID, 0);
		static float scale = 1.2;
		static bool test = true;
		if (test)
		{
			auto hiZOcclusionCull = jShader::GetShader("HiZOcclusionCull");
			if (hiZOcclusionCull)
			{
				auto hiZOcclusionCull_gl = static_cast<jShader_OpenGL*>(hiZOcclusionCull);

				glUseProgram(hiZOcclusionCull_gl->program);

				auto tex = glGetUniformLocation(hiZOcclusionCull_gl->program, "HiZBuffer");
				glUniform1i(tex, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture_gl->TextureID);

				SET_UNIFORM_BUFFER_STATIC(Vector4, "ViewportSize", Vector4(0.0f, 0.0f, SCR_WIDTH, SCR_HEIGHT), hiZOcclusionCull);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "Color", Vector4(1.0f, 0.0f, 0.0f, 0.0f), hiZOcclusionCull);

				glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, TFBO);
				glBeginTransformFeedback(GL_POINTS);
				
				glEnable(GL_RASTERIZER_DISCARD);
				glBeginQuery(GL_PRIMITIVES_GENERATED, CullQuery);

				auto BoundBox = Game.Cube->RenderObject->BoundBox;
				Game.Cube->RenderObject->BoundBox.Max *= scale;
				Game.Cube->RenderObject->BoundBox.Min *= scale;
				Game.Cube->RenderObject->DrawBoundBox(Game.MainCamera, hiZOcclusionCull);
				////glDrawArrays(GL_POINTS, 0, 1);
				Game.Cube->RenderObject->BoundBox = BoundBox;
				glEndQuery(GL_PRIMITIVES_GENERATED);
				glEndTransformFeedback();

				glFlush();

				float feedback[100];
				glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(feedback), feedback);

				//while(!Result)
				//	glGetQueryObjectiv(CullQuery, GL_QUERY_RESULT_AVAILABLE, &Result);
				glGetQueryObjectiv(CullQuery, GL_QUERY_RESULT, &Result);
				glDisable(GL_RASTERIZER_DISCARD);

			}
		}

		if (!test)
		{
			glBeginQueryIndexed(GL_PRIMITIVES_GENERATED, 0, CullQuery);
			auto boundingBox = jShader::GetShader("BoundingBox");
			if (boundingBox)
			{
				auto boundingBox_gl = static_cast<jShader_OpenGL*>(boundingBox);

				glUseProgram(boundingBox_gl->program);
				auto BoundBox = Game.Cube->RenderObject->BoundBox;
				Game.Cube->RenderObject->BoundBox.Max *= scale;
				Game.Cube->RenderObject->BoundBox.Min *= scale;
				Game.Cube->RenderObject->DrawBoundBox(Game.MainCamera, boundingBox);
				Game.Cube->RenderObject->BoundBox = BoundBox;
			}
			glEndQueryIndexed(GL_PRIMITIVES_GENERATED, 0);
			glGetQueryObjectiv(CullQuery, GL_QUERY_RESULT, &Result);
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
