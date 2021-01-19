#pragma once
#include "Math\Vector.h"
#include "jRHI.h"
#include "jRHI_OpenGL.h"

enum class ELightType
{
	AMBIENT = 0,
	DIRECTIONAL,
	POINT,
	SPOT,	
	MAX
};

struct jShader;
struct jTexture;
class jCamera;
struct jRenderTarget;
struct jMaterialData;
class jDirectionalLight;
class jCascadeDirectionalLight;
class jPointLight;
class jSpotLight;
class jObject;
class jLight;

namespace jLightUtil
{
	void MakeDirectionalLightViewInfo(Vector& outPos, Vector& outTarget, Vector& outUp, const Vector& direction);

	//////////////////////////////////////////////////////////////////////////
	struct jShadowMapArrayData
	{
		jShadowMapArrayData()
		{
			UniformBlock = g_rhi->CreateUniformBufferBlock("ShadowMapBlock");
		}

		jShadowMapArrayData(const char* prefix)
		{
			if (prefix)
				UniformBlock = g_rhi->CreateUniformBufferBlock((std::string(prefix) + "ShadowMapBlock").c_str());
			else
				UniformBlock = g_rhi->CreateUniformBufferBlock("ShadowMapBlock");
		}

		~jShadowMapArrayData()
		{
			delete UniformBlock;
		}

		FORCEINLINE bool IsValid() const { return (ShadowMapRenderTarget && ShadowMapCamera[0] && ShadowMapCamera[1] && ShadowMapCamera[2] && ShadowMapCamera[3] && ShadowMapCamera[4] && ShadowMapCamera[5]); }

		std::shared_ptr<jRenderTarget> ShadowMapRenderTarget;
		std::shared_ptr<jSamplerState> ShadowMapSamplerState;
		jCamera* ShadowMapCamera[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		IUniformBufferBlock* UniformBlock = nullptr;
	};

	static jShadowMapArrayData* CreateShadowMapArray(const Vector& pos);

	//////////////////////////////////////////////////////////////////////////
	struct jShadowMapData
	{
		jShadowMapData()
		{
			UniformBlock = g_rhi->CreateUniformBufferBlock("ShadowMapBlock");
		}

		jShadowMapData(const char* prefix)
		{
			if (prefix)
				UniformBlock = g_rhi->CreateUniformBufferBlock((std::string(prefix) + "ShadowMapBlock").c_str());
			else
				UniformBlock = g_rhi->CreateUniformBufferBlock("ShadowMapBlock");
		}

		~jShadowMapData()
		{
			delete UniformBlock;
		}

		FORCEINLINE bool IsValid() const { return (ShadowMapCamera && ShadowMapRenderTarget); }

		std::shared_ptr<jRenderTarget> ShadowMapRenderTarget;
		std::shared_ptr<jSamplerState> ShadowMapSamplerState;
		jCamera* ShadowMapCamera = nullptr;
		IUniformBufferBlock* UniformBlock = nullptr;

		// todo 정리 필요
		Matrix CascadeLightVP[NUM_CASCADES];
		float CascadeEndsW[NUM_CASCADES] = { 0, };
	};

	static jShadowMapData* CreateShadowMap(const Vector& direction, const Vector& pos);
	static jShadowMapData* CreateCascadeShadowMap(const Vector& direction, const Vector& pos);
}

typedef std::function<void(const jRenderTarget*, int32, const jCamera*, const std::vector<jViewport>& viewports)> RenderToShadowMapFunc;

class jLight
{
public:
	// todo debug object
	static jLight* CreateAmbientLight(const Vector& color, const Vector& intensity);
	static jDirectionalLight* CreateDirectionalLight(const Vector& direction, const Vector& color, const Vector& diffuseIntensity
		, const Vector& specularIntensity, float specularPower);
	static jCascadeDirectionalLight* CreateCascadeDirectionalLight(const Vector& direction, const Vector& color, const Vector& diffuseIntensity
		, const Vector& specularIntensity, float specularPower);
	static jPointLight* CreatePointLight(const Vector& pos, const Vector& color, float maxDistance, const Vector& diffuseIntensity
		, const Vector& specularIntensity, float specularPower);
	static jSpotLight* CreateSpotLight(const Vector& pos, const Vector& direction, const Vector& color, float maxDistance
		, float penumbraRadian, float umbraRadian, const Vector& diffuseIntensity, const Vector& specularIntensity, float specularPower);

	static void BindLights(const std::list<const jLight*>& lights, const jShader* shader);

	jLight() = default;
	jLight(ELightType type) :Type(type) {}
	virtual ~jLight() {}

	virtual void BindLight(const jShader* shader) const {}
	virtual void GetMaterialData(jMaterialData* OutMaterialData) const {}
	virtual const jMaterialData* GetMaterialData() const { return nullptr; }
	virtual jRenderTarget* GetShadowMapRenderTarget() const { return nullptr; }
	virtual std::shared_ptr<jRenderTarget> GetShadowMapRenderTargetPtr() const { return nullptr; }
	virtual jCamera* GetLightCamra(int index = 0) const { return nullptr; }
	virtual void RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const {}
	virtual void Update(float deltaTime) { }

	const ELightType Type = ELightType::MAX;
	jObject* LightDebugObject = nullptr;
};

class jAmbientLight : public jLight
{
public:
	jAmbientLight() : jLight(ELightType::AMBIENT) {}

	struct LightData
	{
		Vector Color;
		Vector Intensity;
	};
	
	LightData Data;
	
	virtual void BindLight(const jShader* shader) const override;

};

class jDirectionalLight : public jLight
{
public:
	jDirectionalLight()
		: jLight(ELightType::DIRECTIONAL)
	{
		LightDataUniformBlock = g_rhi->CreateUniformBufferBlock("DirectionalLightBlock");
	}
	virtual ~jDirectionalLight()
	{
		delete LightDataUniformBlock;
	}

	struct LightData
	{
		Vector Direction;
		float SpecularPow = 0.0f;

		Vector Color;
		float padding0;

		Vector DiffuseIntensity;
		float padding1;

		Vector SpecularIntensity;
		float padding2;

		bool operator == (const LightData& rhs) const
		{
			return (Direction == rhs.Direction && Color == rhs.Color && DiffuseIntensity == rhs.DiffuseIntensity 
				&& SpecularIntensity == rhs.SpecularIntensity && SpecularPow == rhs.SpecularPow);
		}

		bool operator != (const LightData& rhs) const
		{
			return !(*this == rhs);
		}
	};

	LightData Data;
	IUniformBufferBlock* LightDataUniformBlock = nullptr;

	virtual void BindLight(const jShader* shader) const override;
	virtual void GetMaterialData(jMaterialData* OutMaterialData) const override;
	virtual const jMaterialData* GetMaterialData() const override { return &MaterialData; }
	virtual jRenderTarget* GetShadowMapRenderTarget() const override;
	virtual std::shared_ptr<jRenderTarget> GetShadowMapRenderTargetPtr() const override;
	virtual jCamera* GetLightCamra(int index = 0) const;

	jLightUtil::jShadowMapData* ShadowMapData = nullptr;
	jTexture* GetShadowMap() const;

	virtual void RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const override;
	virtual void Update(float deltaTime) override;

	jMaterialData MaterialData;
	bool DirtyMaterialData = true;
	void UpdateMaterialData();
};

class jCascadeDirectionalLight : public jDirectionalLight
{
public:
	virtual void RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const override;
};

class jPointLight : public jLight
{
public:
	jPointLight()
		: jLight(ELightType::POINT) 
	{
		LightDataUniformBlock = g_rhi->CreateUniformBufferBlock("PointLightBlock");
	}

	virtual ~jPointLight()
	{
		delete LightDataUniformBlock;
	}

	struct LightData
	{
		Vector Position;
		float SpecularPow = 0.0f;

		Vector Color;
		float MaxDistance = 0.0f;

		Vector DiffuseIntensity;
		float padding0;
		
		Vector SpecularIntensity;		
		float padding1;

		bool operator == (const LightData& rhs) const
		{
			return (Position == rhs.Position && Color == rhs.Color && DiffuseIntensity == rhs.DiffuseIntensity
				&& SpecularIntensity == rhs.SpecularIntensity && SpecularPow == rhs.SpecularPow && MaxDistance == rhs.MaxDistance);
		}

		bool operator != (const LightData& rhs) const
		{
			return !(*this == rhs);
		}
	};

	LightData Data;
	IUniformBufferBlock* LightDataUniformBlock = nullptr;

	virtual void BindLight(const jShader* shader) const override;
	virtual void GetMaterialData(jMaterialData* OutMaterialData) const override;
	virtual const jMaterialData* GetMaterialData() const override { return &MaterialData; }
	virtual jRenderTarget* GetShadowMapRenderTarget() const override;
	virtual std::shared_ptr<jRenderTarget> GetShadowMapRenderTargetPtr() const override;
	virtual jCamera* GetLightCamra(int index = 0) const;

	jLightUtil::jShadowMapArrayData* ShadowMapData = nullptr;

	virtual void RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const override;
	virtual void Update(float deltaTime) override;

	jMaterialData MaterialData;
	bool DirtyMaterialData = true;
	void UpdateMaterialData();
};

class jSpotLight : public jLight
{
public:
	jSpotLight()
		: jLight(ELightType::SPOT)
	{
		LightDataUniformBlock = g_rhi->CreateUniformBufferBlock("SpotLightBlock");
	}

	virtual ~jSpotLight()
	{
		delete LightDataUniformBlock;
	}

	struct LightData
	{
		Vector Position;
		float MaxDistance = 0.0f;

		Vector Direction;
		float PenumbraRadian = 0.0f;

		Vector Color;
		float UmbraRadian = 0.0f;

		Vector DiffuseIntensity;
		float SpecularPow = 0.0f;

		Vector SpecularIntensity;
		float padding0;

		bool operator == (const LightData& rhs) const
		{
			return (Position == rhs.Position && Direction == rhs.Direction && Color == rhs.Color && DiffuseIntensity == rhs.DiffuseIntensity
				&& SpecularIntensity == rhs.SpecularIntensity && SpecularPow == rhs.SpecularPow && MaxDistance == rhs.MaxDistance
				&& PenumbraRadian == rhs.PenumbraRadian && UmbraRadian == rhs.UmbraRadian);
		}

		bool operator != (const LightData& rhs) const
		{
			return !(*this == rhs);
		}
	};

	LightData Data;
	IUniformBufferBlock* LightDataUniformBlock = nullptr;

	virtual void BindLight(const jShader* shader) const override;
	virtual void GetMaterialData(jMaterialData* OutMaterialData) const override;
	virtual const jMaterialData* GetMaterialData() const override { return &MaterialData; }
	virtual jRenderTarget* GetShadowMapRenderTarget() const override;
	virtual std::shared_ptr<jRenderTarget> GetShadowMapRenderTargetPtr() const override;
	virtual jCamera* GetLightCamra(int index = 0) const;

	jLightUtil::jShadowMapArrayData* ShadowMapData = nullptr;

	virtual void RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const override;
	virtual void Update(float deltaTime) override;

	jMaterialData MaterialData;
	bool DirtyMaterialData = true;
	void UpdateMaterialData();
};