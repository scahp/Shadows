#include "pch.h"
#include "jShader.h"
#include "jFile.h"
#include "jRHI.h"

//////////////////////////////////////////////////////////////////////////
// jShaderInfo
std::map<std::string, jShaderInfo> jShaderInfo::s_ShaderInfoMap;
void jShaderInfo::AddShaderInfo(const jShaderInfo& shaderInfo)
{
	s_ShaderInfoMap.insert(std::make_pair(shaderInfo.name, shaderInfo));
}

void jShaderInfo::CreateShaders()
{
	for (auto& it : s_ShaderInfoMap)
		jShader::CreateShader(it.second);
}

struct jShaderInfoCreation
{
	jShaderInfoCreation()
	{
		// Declare shader
		DECLARE_SHADER_VS_FS("Simple", "Shaders/color_only_vs.glsl", "Shaders/color_only_fs.glsl");
		DECLARE_SHADER_VS_FS("ShadowMap", "Shaders/vs_shadowMap.glsl", "Shaders/fs_shadowMap.glsl");
		DECLARE_SHADER_VS_FS("Base", "Shaders/shadowmap/vs.glsl", "Shaders/shadowmap/fs.glsl");
		DECLARE_SHADER_VS_FS_WITH_OPTION("BaseTexture", "Shaders/shadowmap/vs.glsl", "Shaders/shadowmap/fs.glsl", true, true);
		DECLARE_SHADER_VS_GS_FS("ShadowMapOmni", "Shaders/shadowmap/vs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/gs_omniDirectionalShadowMap.glsl", "Shaders/shadowmap/fs_omniDirectionalShadowMap.glsl");
		DECLARE_SHADER_VS_FS("ShadowVolumeBase", "Shaders/shadowvolume/vs.glsl", "Shaders/shadowvolume/fs.glsl");
		DECLARE_SHADER_VS_FS("ExpDeepShadowMapGen", "Shaders/shadowmap/vs_expDeepShadowMap.glsl", "Shaders/shadowmap/fs_expDeepShadowMap.glsl");
		DECLARE_SHADER_VS_FS("DeepShadowMapGen", "Shaders/shadowmap/vs_shadowMap.glsl", "Shaders/shadowmap/fs_deepShadowMap.glsl");
		DECLARE_SHADER_VS_FS_WITH_OPTION("Hair", "shaders/shadowmap/vs_hair.glsl", "shaders/shadowmap/fs_hair.glsl", true, true);
		DECLARE_SHADER_VS_FS_WITH_OPTION("ExpDeepShadowFull", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_expdeepshadow.glsl", true, true);
		DECLARE_SHADER_VS_FS("DeepShadowFull", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_deepshadow.glsl");
		DECLARE_SHADER_VS_FS("DeepShadowAA", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_deepshadow_aa.glsl");
		DECLARE_SHADER_VS_FS_WITH_OPTION("ExpDeferred", "shaders/shadowmap/vs_deferred.glsl", "shaders/shadowmap/fs_expDeferred.glsl", true, true);
		DECLARE_SHADER_VS_FS_WITH_OPTION("Deferred", "shaders/shadowmap/vs_deferred.glsl", "shaders/shadowmap/fs_deferred.glsl", true, true);
		DECLARE_SHADER_CS("cs", "Shaders/compute/compute_example.glsl");
		DECLARE_SHADER_CS("cs_sort", "Shaders/compute/compute_sort_linkedlist.glsl");
		DECLARE_SHADER_CS("cs_link", "Shaders/compute/compute_link_linkedlist.glsl");

		DECLARE_SHADER_VS_FS("ShadowGen_SSM", "shaders/shadowmap/vs_shadowMap.glsl", "shaders/shadowmap/fs_shadowMap.glsl");
		DECLARE_SHADER_VS_GS_FS("ShadowGen_Omni_SSM", "shaders/shadowmap/vs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/gs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/fs_omniDirectionalShadowMap.glsl");
		DECLARE_SHADER_VS_FS("SSM", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl");
		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("SSM_PCF", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl", false, false, "\r\n#define USE_PCF 1");
		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("SSM_PCF_Poisson", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl", false, false, "\r\n#define USE_PCF 1\r\n#define USE_POISSON_SAMPLE 1");

		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("SSM_PCSS", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl", false, false, "\r\n#define USE_PCSS 1");
		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("SSM_PCSS_Poisson", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl", false, false, "\r\n#define USE_PCSS 1\r\n#define USE_POISSON_SAMPLE 1");

		DECLARE_SHADER_VS_FS("ShadowGen_VSM", "shaders/shadowmap/vs_varianceShadowMap.glsl", "shaders/shadowmap/fs_varianceShadowMap.glsl");
		DECLARE_SHADER_VS_GS_FS("ShadowGen_Omni_VSM", "shaders/shadowmap/vs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/gs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/fs_omniDirectionalShadowMap.glsl");

		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("VSM", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl", false, false, "\r\n#define USE_VSM 1");
		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("ESM", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl", false, false, "\r\n#define USE_ESM 1");
		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("EVSM", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl", false, false, "\r\n#define USE_EVSM 1");

		DECLARE_SHADER_VS_FS("Blur", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_blur.glsl");
		DECLARE_SHADER_VS_FS("BlurOmni", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_omnidirectional_blur.glsl");
		DECLARE_SHADER_VS_FS("Tonemap", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_tonemap.glsl");

		DECLARE_SHADER_VS_FS("ShadowGen_ESM", "shaders/shadowmap/vs_varianceShadowMap.glsl", "shaders/shadowmap/fs_exponentialShadowMap.glsl");
		DECLARE_SHADER_VS_GS_FS("ShadowGen_Omni_ESM", "shaders/shadowmap/vs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/gs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/fs_omniDirectionalExponentialShadowMap.glsl");

		DECLARE_SHADER_VS_FS("ShadowGen_EVSM", "shaders/shadowmap/vs_EVSM.glsl", "shaders/shadowmap/fs_EVSM.glsl");
		DECLARE_SHADER_VS_GS_FS("ShadowGen_Omni_EVSM", "shaders/shadowmap/vs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/gs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/fs_omniDirectionalEVSM.glsl");

		DECLARE_SHADER_VS_FS("BoundVolumeShader", "Shaders/vs_boundvolume.glsl", "Shaders/fs_boundvolume.glsl");
		DECLARE_SHADER_VS_FS("DebugObjectShader", "Shaders/tex_vs.glsl", "Shaders/tex_fs.glsl");
		DECLARE_SHADER_VS_FS("UIShader", "Shaders/tex_ui_vs.glsl", "Shaders/tex_ui_fs.glsl");
		
		DECLARE_SHADER_VS_FS("AmbientOnly", "Shaders/shadowvolume/vs.glsl", "Shaders/shadowvolume/fs_ambientonly.glsl");
		DECLARE_SHADER_VS_FS("ShadowVolume", "Shaders/shadowvolume/vs.glsl", "Shaders/shadowvolume/fs.glsl");
		DECLARE_SHADER_VS_FS("ShadowVolumeCPU", "Shaders/shadowvolume/vs_shadowvolume_cpu.glsl", "Shaders/shadowvolume/fs_shadowvolume_cpu.glsl");
		DECLARE_SHADER_VS_GS_FS("ShadowVolumeGPU", "Shaders/shadowvolume/vs_shadowvolume_gpu.glsl", "Shaders/shadowvolume/gs_shadowvolume_gpu.glsl", "Shaders/shadowvolume/fs_shadowvolume_gpu.glsl");

		DECLARE_SHADER_VS_FS("RedShader", "Shaders/vs_red.glsl", "Shaders/fs_red.glsl");

		//DECLARE_SHADER_VS_FS("CSM_SSM", "Shaders/vs_test.glsl", "Shaders/fs_test.glsl");
		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("CSM_SSM", "shaders/shadowmap/vs.glsl", "shaders/shadowmap/fs.glsl", false, false, "\r\n#define USE_CSM 1");

		DECLARE_SHADER_VS_GS_FS("CSM_SSM_TEX2D_ARRAY", "shaders/shadowmap/vs_omniDirectionalShadowMap.glsl", "shaders/shadowmap/gs_cascadeShadowMap.glsl", "shaders/shadowmap/fs_shadowMap.glsl");

		DECLARE_SHADER_VS_FS("LuminanceMapGeneration", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_luminanceMap.glsl");
		DECLARE_SHADER_VS_FS("AdaptiveLuminance", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_adaptiveLuminance.glsl");
		DECLARE_SHADER_VS_FS("BloomThreshold", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_bloom_threshold.glsl");
		DECLARE_SHADER_VS_FS("Scale", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_scale.glsl");
		DECLARE_SHADER_VS_FS("GaussianBlurH", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_blur_gaussian_horizontal.glsl");
		DECLARE_SHADER_VS_FS("GaussianBlurV", "shaders/fullscreen/vs_fullscreen_common.glsl", "shaders/fullscreen/fs_blur_gaussian_vertical.glsl");

		DECLARE_SHADER_VS_FS("Graph2D", "shaders/vs_graph.glsl", "shaders/fs_graph.glsl");

		//////////////////////////////////////////////////////////////////////////
		DECLARE_SHADER_VS_FS("NewDepthOnly", "Shaders/NewShaders/depthonly_vs.glsl", "Shaders/NewShaders/empty_fs.glsl");
		DECLARE_SHADER_VS_FS("NewDepthOnlyWithAlphaTest", "Shaders/NewShaders/depthonly_alphatest_vs.glsl", "Shaders/NewShaders/depthonly_alphatest_fs.glsl");
		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("NewDeferred", "Shaders/NewShaders/deferred_vs.glsl", "Shaders/NewShaders/deferred_fs.glsl", true, true, );
		DECLARE_SHADER_VS_FS_WITH_OPTION_MORE("NewDeferredLighting", "Shaders/NewShaders/deferred_lighting_vs.glsl", "Shaders/NewShaders/deferred_lighting_fs.glsl", true, false, );
		DECLARE_SHADER_VS_FS("NewSSM", "Shaders/NewShaders/SSM_vs.glsl", "Shaders/NewShaders/SSM_fs.glsl");
		DECLARE_SHADER_VS_FS("NewSSAO", "Shaders/NewShaders/ssao_vs.glsl", "Shaders/NewShaders/ssao_fs.glsl");
		DECLARE_SHADER_VS_FS("NewSSAOBlur", "Shaders/NewShaders/ssao_blur_vs.glsl", "Shaders/NewShaders/ssao_blur_fs.glsl");
		DECLARE_SHADER_VS_FS_WITH_DEFINE("NewSSR_Linear", "Shaders/NewShaders/SSR_vs.glsl", "Shaders/NewShaders/SSR_fs.glsl", "");
		DECLARE_SHADER_VS_FS_WITH_DEFINE("NewSSR_HiZ", "Shaders/NewShaders/SSR_vs.glsl", "Shaders/NewShaders/SSR_fs.glsl", "#define USE_HIZ 1");
		DECLARE_SHADER_VS_FS("NewCreateHiZ", "Shaders/NewShaders/CreateHiZ_vs.glsl", "Shaders/NewShaders/CreateHiZ_fs.glsl");
		DECLARE_SHADER_VS_FS("NewDepthCopy", "shaders/NewShaders/depthcopy_vs.glsl", "shaders/NewShaders/depthcopy_fs.glsl");
		DECLARE_SHADER_VS_FS("NewCopy", "shaders/NewShaders/copy_vs.glsl", "shaders/NewShaders/copy_fs.glsl");
		DECLARE_SHADER_VS_FS("NewFXAA", "shaders/NewShaders/FXAA_vs.glsl", "shaders/NewShaders/FXAA_fs.glsl");
		//DECLARE_SHADER_VS_FS("NewPPR_ProjectionPass", "shaders/NewShaders/PPR_ProjectionPass_vs.glsl", "shaders/NewShaders/PPR_ProjectionPass_fs.glsl");
		//DECLARE_SHADER_VS_FS("NewPPR_ReflectionPass", "shaders/NewShaders/PPR_ReflectionPass_vs.glsl", "shaders/NewShaders/PPR_ReflectionPass_fs.glsl");
		DECLARE_SHADER_CS("NewPPR_ProjectionPass", "Shaders/NewShaders/PPR_ProjectionPass_cs.glsl");
		DECLARE_SHADER_CS("NewPPR_ReflectionPass", "Shaders/NewShaders/PPR_ReflectionPass_cs.glsl");
		DECLARE_SHADER_CS("NewPPR_ClearIntermediateBuffer", "Shaders/NewShaders/PPR_ClearIntermediateBuffer_cs.glsl");
		DECLARE_SHADER_VS_FS("NewTonemap", "shaders/NewShaders/tonemap_vs.glsl", "shaders/NewShaders/tonemap_fs.glsl");
	}
} s_shaderInfoCreation;

//////////////////////////////////////////////////////////////////////////
std::unordered_map<size_t, std::shared_ptr<jShader> > jShader::ShaderMap;
std::vector<std::shared_ptr<jShader> > jShader::ShaderVector;
std::unordered_map < std::string, std::shared_ptr<jShader> > jShader::ShaderNameMap;


jShader* jShader::GetShader(size_t hashCode)
{
	auto it_find = ShaderMap.find(hashCode);
	if (it_find != ShaderMap.end())
		return it_find->second.get();
	return nullptr;
}

std::shared_ptr<jShader> jShader::GetShaderPtr(size_t hashCode)
{
	auto it_find = ShaderMap.find(hashCode);
	if (it_find != ShaderMap.end())
		return it_find->second;
	return nullptr;
}


jShader* jShader::GetShader(const std::string& name)
{
	auto it_find = ShaderNameMap.find(name);
	if (it_find != ShaderNameMap.end())
		return it_find->second.get();
	return nullptr;
}

std::shared_ptr<jShader> jShader::GetShaderPtr(const std::string& name)
{
	auto it_find = ShaderNameMap.find(name);
	if (it_find != ShaderNameMap.end())
		return it_find->second;
	return nullptr;
}

jShader* jShader::CreateShader(const jShaderInfo& shaderInfo)
{
	auto shaderPtr = CreateShaderPtr(shaderInfo);
	return (shaderPtr ? shaderPtr.get() : nullptr);
}

std::shared_ptr<jShader> jShader::CreateShaderPtr(const jShaderInfo& shaderInfo)
{
	auto hash = shaderInfo.CreateShaderHash();
	auto shaderPtr = GetShaderPtr(hash);
	if (shaderPtr)
	{
		if (ShaderNameMap.end() == ShaderNameMap.find(shaderInfo.name))
			ShaderNameMap.insert(std::make_pair(shaderInfo.name, shaderPtr));
		else
			JASSERT(!"The shader was already created before.");
		return shaderPtr;
	}

	shaderPtr = std::shared_ptr<jShader>(g_rhi->CreateShader(shaderInfo));

	if (!shaderPtr)
		return nullptr;

	ShaderMap[hash] = shaderPtr;
	if (!shaderInfo.name.empty())
	{
		JASSERT(ShaderNameMap.end() == ShaderNameMap.find(shaderInfo.name));
		ShaderNameMap[shaderInfo.name] = shaderPtr;
	}
	ShaderVector.push_back(shaderPtr);
	return shaderPtr;
}

void jShader::UpdateShaders()
{
	if (ShaderVector.empty())
		return;

	static uint64 currentFrame = 0;
	currentFrame = (currentFrame + 1) % 5;
	if (currentFrame)
		return;

	static uint64 currentFile = 0;
	currentFile = (currentFile + 1) % ShaderVector.size();
	const std::shared_ptr<jShader>& currentShader = ShaderVector[currentFile];
	JASSERT(currentShader);
	currentShader->UpdateShader();
}

bool jShader::SetUniformbuffer(const char* name, const Matrix& InData) const
{
	return g_rhi->SetUniformbuffer(name, InData, this);
}

bool jShader::SetUniformbuffer(const char* name, const bool InData) const
{
	return g_rhi->SetUniformbuffer(name, InData, this);
}

bool jShader::SetUniformbuffer(const char* name, const float InData) const
{
	return g_rhi->SetUniformbuffer(name, InData, this);
}

bool jShader::SetUniformbuffer(const char* name, const int32 InData) const
{
	return g_rhi->SetUniformbuffer(name, InData, this);
}

bool jShader::SetUniformbuffer(const char* name, const Vector2& InData) const
{
	return g_rhi->SetUniformbuffer(name, InData, this);
}

bool jShader::SetUniformbuffer(const char* name, const Vector& InData) const
{
	return g_rhi->SetUniformbuffer(name, InData, this);
}

bool jShader::SetUniformbuffer(const char* name, const Vector4& InData) const
{
	return g_rhi->SetUniformbuffer(name, InData, this);
}

void jShader::UpdateShader()
{
	auto checkTimeStampFunc = [this](const std::string& filename) -> uint64
	{
		if (filename.length() > 0)
			return jFile::GetFileTimeStamp(filename.c_str());
		return 0;
	};

	uint64 currentTimeStamp = checkTimeStampFunc(ShaderInfo.vs);
	currentTimeStamp = Max(currentTimeStamp, checkTimeStampFunc(ShaderInfo.fs));
	currentTimeStamp = Max(currentTimeStamp, checkTimeStampFunc(ShaderInfo.cs));
	currentTimeStamp = Max(currentTimeStamp, checkTimeStampFunc(ShaderInfo.gs));
	
	if (currentTimeStamp <= 0)
		return;
	
	if (TimeStamp == 0)
	{
		TimeStamp = currentTimeStamp;
		return;
	}

	if (TimeStamp < currentTimeStamp)
	{
		g_rhi->CreateShader(this, ShaderInfo);
		TimeStamp = currentTimeStamp;
	}
}
