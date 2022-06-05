#pragma once
#include "jBoundPrimitiveType.h"

class jRenderObject;
class jCamera;
struct jShader;
class jLight;
class jVertexAdjacency;
class IShadowVolume;

class jObject
{
public:
	jObject();
	virtual ~jObject();

	//////////////////////////////////////////////////////////////////////////
	static void AddObject(jObject* object);
	static void RemoveObject(jObject* object);
	static void FlushDirtyState();

	static const std::list<jObject*>& GetShadowCasterObject() { return s_ShadowCasterObject; }
	static const std::list<jObject*>& GetStaticObject() { return s_StaticObjects; }
	static const std::list<jObject*>& GetBoundBoxObject() { return s_BoundBoxObjects; }
	static const std::list<jObject*>& GetBoundSphereObject() { return s_BoundSphereObjects; }
	static const std::list<jObject*>& GetDebugObject() { return s_DebugObjects; }
	static const std::list<jObject*>& GetUIObject() { return s_UIObjects; }
	static const std::list<jObject*>& GetUIDebugObject() { return s_UIDebugObjects; }

	static void AddBoundBoxObject(jObject* object);
	static void RemoveBoundBoxObject(jObject* object);
	
	static void AddBoundSphereObject(jObject* object);
	static void RemoveBoundSphereObject(jObject* object);

	static void AddDebugObject(jObject* object);
	static void RemoveDebugObject(jObject* object);

	static void AddUIObject(jObject* object);
	static void RemoveUIObject(jObject* object);

	static void AddUIDebugObject(jObject* object);
	static void RemoveUIDebugObject(jObject* object);
	//////////////////////////////////////////////////////////////////////////

	virtual void Update(float deltaTime);
	//virtual void Draw(const jCamera* camera, const jShader* shader);
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const;

	void SetDirtyState() { DirtyObjectState = true; s_DirtyStateObjects.insert(this); }

	void SetSkipShadowMapGen(bool skipShadowMapGen) { SkipShadowMapGen = skipShadowMapGen; SetDirtyState(); }
	void SetSkipUpdateShadowVolume(bool skipUpdateShadowVolume) { SkipUpdateShadowVolume = skipUpdateShadowVolume; SetDirtyState(); }
	void SetVisible(bool visible) { Visible = visible; SetDirtyState(); }

	void CreateBoundBox(bool isShow = true);
	void ShowBoundBox(bool isShow);

	jRenderObject* RenderObject = nullptr;
	jVertexAdjacency* VertexAdjacency = nullptr;
	IShadowVolume* ShadowVolumeGPU = nullptr;
	IShadowVolume* ShadowVolumeCPU = nullptr;

	bool SkipShadowMapGen = false;
	bool SkipUpdateShadowVolume = false;
	bool Visible = true;
	bool DirtyObjectState = false;
	
	bool IsPostUpdate = true;
	std::function<void(jObject*, float)> PostUpdateFunc;

	// todo 현재는 보유만 하고있음.
	jObject* BoundBoxObject = nullptr;
	jObject* BoundSphereObject = nullptr;

	jBoundBox BoundBox;
	jBoundSphere BoundSphere;

private:
	static std::list<jObject*> s_ShadowCasterObject;
	static std::list<jObject*> s_StaticObjects;
	static std::list<jObject*> s_BoundBoxObjects;
	static std::list<jObject*> s_BoundSphereObjects;
	static std::list<jObject*> s_DebugObjects;
	static std::list<jObject*> s_UIObjects;
	static std::list<jObject*> s_UIDebugObjects;
	static std::set<jObject*> s_DirtyStateObjects;
};

