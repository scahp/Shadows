﻿#pragma once

//////////////////////////////////////////////////////////////////////////
// jQueryPool_Vulkan
//////////////////////////////////////////////////////////////////////////
struct jQueryPool_Vulkan : public jQueryPool
{
    virtual ~jQueryPool_Vulkan() {}

    virtual bool Create() override;
    virtual void ResetQueryPool(jCommandBuffer* pCommanBuffer = nullptr);

    VkQueryPool vkQueryPool = nullptr;
    int32 QueryIndex[jRHI::MaxWaitingQuerySet] = { 0, };
};

//////////////////////////////////////////////////////////////////////////
// jQueryTime_Vulkan
//////////////////////////////////////////////////////////////////////////
struct jQueryTime_Vulkan : public jQueryTime
{
    virtual ~jQueryTime_Vulkan() {}
    virtual void Init() override;

    uint32 QueryId = 0;
};