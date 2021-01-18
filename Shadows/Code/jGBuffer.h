#pragma once

struct jRenderTarget;
struct jShader;

class jGBuffer
{
public:
	~jGBuffer();

	bool Begin(int index = 0, bool mrt = false) const;
	void End() const;
	void BindGeometryBuffer(const jShader* shader) const;

	std::shared_ptr<jRenderTarget> GeometryBuffer;
};