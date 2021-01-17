#include "pch.h"
#include "jPerformanceProfile.h"

jPerformanceProfile* jPerformanceProfile::_instance = nullptr;

std::map<std::string, uint64> ScopedProfileCPUMap[MaxProfileFrame];
std::map<std::string, uint64> ScopedProfileGPUMap[MaxProfileFrame];
static int32 PerformanceFrame = 0;

std::set<jQueryTime*> jQueryTimePool::s_running;
std::list<jQueryTime*> jQueryTimePool::s_resting;

std::list<jProfile_GPU> jProfile_GPU::WatingResultList[2];
int32 jProfile_GPU::CurrentWatingResultListIndex = 0;

void ClearFramePerformanceData(int32 frameIndex)
{
	ScopedProfileCPUMap[frameIndex].clear();
	ScopedProfileGPUMap[frameIndex].clear();
}

int32 NextFrame()
{
	PerformanceFrame = (PerformanceFrame + 1) % MaxProfileFrame;
	return PerformanceFrame;
}

void ClearScopedProfileCPU()
{
	for (int32 i = 0; i < MaxProfileFrame; ++i)
		ScopedProfileCPUMap[i].clear();
}

void AddScopedProfileCPU(const std::string& name, uint64 elapsedTick)
{
	const bool AlreadyHas = ScopedProfileCPUMap[PerformanceFrame].find(name) != ScopedProfileCPUMap[PerformanceFrame].end();
	if (AlreadyHas)
		ScopedProfileCPUMap[PerformanceFrame][name] += elapsedTick;
	else
		ScopedProfileCPUMap[PerformanceFrame][name] = elapsedTick;
}

void ClearScopedProfileGPU()
{
	for (int32 i = 0; i < MaxProfileFrame; ++i)
		ScopedProfileGPUMap[i].clear();
}

void AddScopedProfileGPU(const std::string& name, uint64 elapsedTick)
{
	const bool AlreadyHas = ScopedProfileGPUMap[PerformanceFrame].find(name) != ScopedProfileGPUMap[PerformanceFrame].end();
	if (AlreadyHas)
		ScopedProfileGPUMap[PerformanceFrame][name] += elapsedTick;
	else
		ScopedProfileGPUMap[PerformanceFrame][name] = elapsedTick;
}

void jPerformanceProfile::Update(float deltaTime)
{
	jProfile_GPU::ProcessWaitings();

	CalcAvg();

	if (TRUE_PER_MS(1000))
		PrintOutputDebugString();

	const int32 NewFrameIndex = NextFrame();
	ClearFramePerformanceData(NewFrameIndex);
}

void jPerformanceProfile::CalcAvg()
{
	{
		AvgProfileMap.clear();

		for (int32 i = 0; i < MaxProfileFrame; ++i)
		{
			const auto& scopedProfileMap = ScopedProfileCPUMap[i];
			for (auto& iter : scopedProfileMap)
			{
				auto& avgProfile = AvgProfileMap[iter.first];
				avgProfile.TotalElapsedTick += iter.second;
				++avgProfile.TotalSampleCount;
			}
		}

		for (auto& iter : AvgProfileMap)
		{
			JASSERT(iter.second.TotalSampleCount > 0);
			iter.second.AvgElapsedMS = iter.second.TotalElapsedTick / static_cast<double>(iter.second.TotalSampleCount);	// ms
		}
	}

	{
		GPUAvgProfileMap.clear();

		for (int32 i = 0; i < MaxProfileFrame; ++i)
		{
			const auto& scopedProfileMap = ScopedProfileGPUMap[i];
			for (auto& iter : scopedProfileMap)
			{
				auto& avgProfile = GPUAvgProfileMap[iter.first];
				avgProfile.TotalElapsedTick += iter.second;
				++avgProfile.TotalSampleCount;
			}
		}

		for (auto& iter : GPUAvgProfileMap)
		{
			JASSERT(iter.second.TotalSampleCount > 0);
			iter.second.AvgElapsedMS = (iter.second.TotalElapsedTick / static_cast<double>(iter.second.TotalSampleCount)) * 0.000001;	// ns -> ms
		}
	}
}

void jPerformanceProfile::PrintOutputDebugString()
{
	std::string result;
	char szTemp[128] = { 0, };
	if (!AvgProfileMap.empty())
	{
		result += "-----CPU---PerformanceProfile----------\n";
		for (auto& iter : AvgProfileMap)
		{
			sprintf_s(szTemp, sizeof(szTemp), "%s : \t\t\t\t%lf ms", iter.first.c_str(), iter.second.AvgElapsedMS);
			result += szTemp;
			result += "\n";
		}
	}

	if (!GPUAvgProfileMap.empty())
	{
		result += "-----GPU---PerformanceProfile----------\n";
		for (auto& iter : GPUAvgProfileMap)
		{
			sprintf_s(szTemp, sizeof(szTemp), "%s : \t\t\t\t%lf ms", iter.first.c_str(), iter.second.AvgElapsedMS);
			result += szTemp;
			result += "\n";
		}
	}
	OutputDebugStringA(result.c_str());
}
