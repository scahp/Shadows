#pragma once
#include "Math/Vector.h"
#include "jPipeline.h"

class jVertexAdjacency;
class jObject;
struct jVertexStreamData;

//////////////////////////////////////////////////////////////////////////
// IShadowVolume
class IShadowVolume
{
public:	
	IShadowVolume() = default;
	IShadowVolume(jVertexAdjacency* vertexAdjacency) : VertexAdjacency(vertexAdjacency) {}

	virtual void Update(const Vector& lightPosOrDirection, bool isOmniDirectional, jObject* ownerObject) {}
	virtual void UpdateShadowVolumeObject() {}

	virtual void CreateShadowVolumeObject() = 0;
	virtual void Draw(const jPipelineContext& pipelineContext, const jShader* shader) const = 0;

protected:
	jVertexAdjacency* VertexAdjacency = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// jShadowVolumeCPU
class jShadowVolumeCPU : public IShadowVolume
{
public:
	jShadowVolumeCPU() = default;
	using IShadowVolume::IShadowVolume;

	virtual void Update(const Vector& lightPosOrDirection, bool isOmniDirectional, jObject* ownerObject) override;
	virtual void Draw(const jPipelineContext& pipelineContext, const jShader* shader) const override;

	jObject* EdgeObject = nullptr;
	jObject* QuadObject = nullptr;

private:
	void UpdateEdge(size_t edgeKey);		// to do the edge detection
	virtual void CreateShadowVolumeObject() override;
	virtual void UpdateShadowVolumeObject() override;

private:
	bool IsInitialized = false;
	bool CreateEdgeObject = false;
	bool CreateQuadObject = true;
	std::set<size_t> Edges;
	std::vector<float> EdgeVertices;
	std::vector<float> QuadVertices;

	std::shared_ptr<jVertexStreamData> EdgeVertexStreamData;
	std::shared_ptr<jVertexStreamData> QuadVertexStreamData;
};

//////////////////////////////////////////////////////////////////////////
// jShadowVolumeGPU
class jShadowVolumeGPU : public IShadowVolume
{
public:
	jShadowVolumeGPU() = default;
	using IShadowVolume::IShadowVolume;

	virtual void CreateShadowVolumeObject() override;
	virtual void Update(const Vector& lightPosOrDirection, bool isOmniDirectional, jObject* ownerObject) override;
	virtual void Draw(const jPipelineContext& pipelineContext, const jShader* shader) const override;

	jObject* ShadowVolumeObject = nullptr;
};