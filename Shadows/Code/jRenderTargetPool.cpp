#include "pch.h"
#include "jRenderTargetPool.h"
#include "jRHI.h"

std::map<size_t, std::list<jRenderTargetPool::jRenderTargetPoolResource> > jRenderTargetPool::RenderTargetResourceMap;
std::map<jRenderTarget*, size_t> jRenderTargetPool::RenderTargetHashVariableMap;

struct jTexture* jRenderTargetPool::GetNullTexture(ETextureType type)
{
	switch (type)
	{
	case ETextureType::TEXTURE_2D:
	case ETextureType::TEXTURE_2D_ARRAY:
	case ETextureType::TEXTURE_2D_ARRAY_OMNISHADOW:
	{
		static auto temp = jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA, ETextureFormat::RGBA, EFormatType::BYTE, EDepthBufferType::NONE, 2, 2, 1 });
		return temp->GetTexture();
	}
	case ETextureType::TEXTURE_CUBE:
	{
		static auto temp = jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_CUBE, ETextureFormat::RGBA, ETextureFormat::RGBA, EFormatType::BYTE, EDepthBufferType::NONE, 2, 2, 1 });
		return temp->GetTexture();
	}
	default:
		JASSERT(0);
		break;
	}

	return nullptr;
}

jRenderTargetPool::jRenderTargetPool()
{
}


jRenderTargetPool::~jRenderTargetPool()
{
}

std::shared_ptr<jRenderTarget> jRenderTargetPool::GetRenderTarget(const jRenderTargetInfo& info)
{
	auto hash = info.GetHash();

	auto it_find = RenderTargetResourceMap.find(hash);
	if (RenderTargetResourceMap.end() != it_find)
	{
		auto& renderTargets = it_find->second;
		for (auto& iter : renderTargets)
		{
			if (!iter.IsUsing)
			{
				iter.IsUsing = true;
				return iter.RenderTargetPtr;
			}
		}
	}

	auto renderTargetPtr = std::shared_ptr<jRenderTarget>(g_rhi->CreateRenderTarget(info));
	if (renderTargetPtr)
	{
		RenderTargetResourceMap[hash].push_back({ true, renderTargetPtr });
		RenderTargetHashVariableMap[renderTargetPtr.get()] = hash;
	}
	
	return renderTargetPtr;
}

void jRenderTargetPool::ReturnRenderTarget(jRenderTarget* renderTarget)
{
	auto it_find = RenderTargetHashVariableMap.find(renderTarget);
	if (RenderTargetHashVariableMap.end() == it_find)
		return;

	const size_t hash = it_find->second;
	for (auto& iter : RenderTargetResourceMap[hash])
	{
		if (renderTarget == iter.RenderTargetPtr.get())
		{
			iter.IsUsing = false;
			break;
		}
	}
}
