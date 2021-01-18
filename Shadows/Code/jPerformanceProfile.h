#pragma once
#include <sysinfoapi.h>
#include "jRHI.h"

static constexpr int32 MaxProfileFrame = 10;

extern std::map<std::string, uint64> ScopedProfileCPUMap[MaxProfileFrame];
extern std::map<std::string, uint64> ScopedProfileGPUMap[MaxProfileFrame];
struct jQueryTime;

void ClearScopedProfileCPU();
void AddScopedProfileCPU(const std::string& name, uint64 elapsedTick);

void ClearScopedProfileGPU();
void AddScopedProfileGPU(const std::string& name, uint64 elapsedTick);

//////////////////////////////////////////////////////////////////////////
// jScopedProfile_CPU
class jScopedProfile_CPU
{
public:
	jScopedProfile_CPU(const std::string& name)
		: Name(name)
	{
		StartTick = GetTickCount64();
	}

	~jScopedProfile_CPU()
	{
		AddScopedProfileCPU(Name, GetTickCount64() - StartTick);
	}

	std::string Name;
	uint64 StartTick = 0;
};

#define SCOPE_PROFILE(Name) jScopedProfile_CPU Name##ScopedProfileCPU(#Name);

//////////////////////////////////////////////////////////////////////////
// jQueryTimePool
struct jQueryTimePool
{
public:
	static jQueryTime* GetQueryTime()
	{
		jQueryTime* queryTime = nullptr;
		if (s_resting.empty())
		{
			queryTime = g_rhi->CreateQueryTime();
		}
		else
		{
			queryTime = s_resting.back();
			s_resting.pop_back();
		}
		s_running.insert(queryTime);
		JASSERT(queryTime);
		return queryTime;
	}
	static void ReturnQueryTime(jQueryTime* queryTime)
	{
		s_running.erase(queryTime);
		s_resting.push_back(queryTime);
	}
	static void DeleteAllQueryTime()
	{
		JASSERT(s_running.empty());
		for (auto& iter : s_resting)
		{
			g_rhi->ReleaseQueryTime(iter);
			delete iter;
		}
		s_resting.clear();
	}

private:
	static std::set<jQueryTime*> s_running;
	static std::list<jQueryTime*> s_resting;
};

//////////////////////////////////////////////////////////////////////////
// jProfile_GPU
struct jProfile_GPU
{
	std::string Name;
	jQueryTime* Start = nullptr;
	jQueryTime* End = nullptr;

	static int32 CurrentWatingResultListIndex;
	static std::list<jProfile_GPU> WatingResultList[2];
	static void ProcessWaitings()
	{
		const auto prevIndex = !CurrentWatingResultListIndex;
		auto& prevList = WatingResultList[prevIndex];
		for (auto rit = prevList.rbegin(); prevList.rend() != rit;++rit)
		{
			const auto& iter = *rit;
			g_rhi->IsQueryTimeStampResult(iter.End, true);
			g_rhi->GetQueryTimeStampResult(iter.End);
			g_rhi->IsQueryTimeStampResult(iter.Start, true);
			g_rhi->GetQueryTimeStampResult(iter.Start);
			AddScopedProfileGPU(iter.Name, iter.End->TimeStamp - iter.Start->TimeStamp);
			jQueryTimePool::ReturnQueryTime(iter.Start);
			jQueryTimePool::ReturnQueryTime(iter.End);
		}
		prevList.clear();
		CurrentWatingResultListIndex = prevIndex;
	}
};

//////////////////////////////////////////////////////////////////////////
// jScopedProfile_GPU
class jScopedProfile_GPU
{
public:
	jScopedProfile_GPU(const std::string& name)
	{
		Profile.Name = name;
		Profile.Start = jQueryTimePool::GetQueryTime();
		JASSERT(Profile.Start);
		g_rhi->QueryTimeStamp(Profile.Start);
	}

	~jScopedProfile_GPU()
	{
		Profile.End = jQueryTimePool::GetQueryTime();
		g_rhi->QueryTimeStamp(Profile.End);
		
		jProfile_GPU::WatingResultList[jProfile_GPU::CurrentWatingResultListIndex].push_back(Profile);
	}

	jProfile_GPU Profile;
};

#define SCOPE_GPU_PROFILE(Name) jScopedProfile_GPU Name##ScopedProfileGPU(#Name);

//////////////////////////////////////////////////////////////////////////
// jPerformanceProfile
class jPerformanceProfile
{
public:
	struct jAvgProfile
	{
		uint64 TotalElapsedTick = 0;
		double AvgElapsedMS = 0;
		int32 TotalSampleCount = 0;
	};

	void Update(float deltaTime);
	void CalcAvg();
	void PrintOutputDebugString();

private:
	std::map<std::string, jAvgProfile> AvgProfileMap;			// ms
	std::map<std::string, jAvgProfile> GPUAvgProfileMap;		// ns

public:
	static jPerformanceProfile& GetInstance()
	{
		if (!_instance)
			_instance = new jPerformanceProfile();
		return *_instance;
	}

private:
	jPerformanceProfile() {}
	static jPerformanceProfile* _instance;
};