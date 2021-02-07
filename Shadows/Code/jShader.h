#pragma once

struct jShaderInfo
{
	static std::map<std::string, jShaderInfo> s_ShaderInfoMap;
	static void AddShaderInfo(const jShaderInfo& shaderInfo);
	static void CreateShaders();

	size_t CreateShaderHash() const
	{
		return std::hash<std::string>{}(vs+vsPreProcessor+gs+gsPreProcessor+fs+fsPreProcessor+cs+csPreProcessor);
	}
	std::string name;
	std::string vs;
	std::string gs;
	std::string fs;
	std::string cs;
	std::string vsPreProcessor;
	std::string gsPreProcessor;
	std::string fsPreProcessor;
	std::string csPreProcessor;
};

#define DECLARE_SHADER_VS_FS_WITH_OPTION_MORE(Name, VS, FS, IsUseTexture, IsUseMaterial, MoreOption) \
{ \
jShaderInfo info; \
info.name = Name; \
info.vs = VS; \
info.fs = FS; \
info.vsPreProcessor += IsUseTexture ? "#define USE_TEXTURE 1" : "";\
info.vsPreProcessor += IsUseMaterial ? "\r\n#define USE_MATERIAL 1" : "";\
info.vsPreProcessor += "\r\n"MoreOption;\
info.fsPreProcessor += IsUseTexture ? "#define USE_TEXTURE 1" : "";\
info.fsPreProcessor += IsUseMaterial ? "\r\n#define USE_MATERIAL 1" : "";\
info.fsPreProcessor += "\r\n"MoreOption;\
jShaderInfo::AddShaderInfo(info); \
}

#define DECLARE_SHADER_VS_FS_WITH_OPTION(Name, VS, FS, IsUseTexture, IsUseMaterial) \
{ \
jShaderInfo info; \
info.name = Name; \
info.vs = VS; \
info.fs = FS; \
info.vsPreProcessor += IsUseTexture ? "#define USE_TEXTURE 1" : "";\
info.vsPreProcessor += IsUseMaterial ? "\r\n#define USE_MATERIAL 1" : "";\
info.fsPreProcessor += IsUseTexture ? "#define USE_TEXTURE 1" : "";\
info.fsPreProcessor += IsUseMaterial ? "\r\n#define USE_MATERIAL 1" : "";\
jShaderInfo::AddShaderInfo(info); \
}

#define DECLARE_SHADER_VS_FS_WITH_DEFINE(Name, VS, FS, AdditionalDifine) \
{ \
jShaderInfo info; \
info.name = Name; \
info.vs = VS; \
info.fs = FS; \
info.vsPreProcessor += "\r\n"AdditionalDifine;\
info.fsPreProcessor += "\r\n"AdditionalDifine;\
jShaderInfo::AddShaderInfo(info); \
}

#define DECLARE_SHADER_VS_FS(Name, VS, FS) DECLARE_SHADER_VS_FS_WITH_OPTION(Name, VS, FS, false, false)

#define DECLARE_SHADER_VS_GS_FS_WITH_OPTION(Name, VS, GS, FS, IsUseTexture, IsUseMaterial) \
{ \
jShaderInfo info; \
info.name = Name; \
info.vs = VS; \
info.gs = GS; \
info.fs = FS; \
info.vsPreProcessor += IsUseTexture ? "#define USE_TEXTURE 1" : "";\
info.vsPreProcessor += IsUseMaterial ? "\r\n#define USE_MATERIAL 1" : "";\
info.gsPreProcessor += IsUseTexture ? "#define USE_TEXTURE 1" : ""; \
info.gsPreProcessor += IsUseMaterial ? "\r\n#define USE_MATERIAL 1" : ""; \
info.fsPreProcessor += IsUseTexture ? "#define USE_TEXTURE 1" : "";\
info.fsPreProcessor += IsUseMaterial ? "\r\n#define USE_MATERIAL 1" : "";\
jShaderInfo::AddShaderInfo(info); \
}

#define DECLARE_SHADER_VS_GS_FS(Name, VS, GS, FS) DECLARE_SHADER_VS_GS_FS_WITH_OPTION(Name, VS, GS, FS, false, false)

#define DECLARE_SHADER_CS_WITH_OPTION(Name, CS, IsUseTexture, IsUseMaterial) \
{ \
jShaderInfo info; \
info.name = Name; \
info.cs = CS; \
info.csPreProcessor += IsUseTexture ? "#define USE_TEXTURE 1" : "";\
info.csPreProcessor += IsUseMaterial ? "\r\n#define USE_MATERIAL 1" : "";\
jShaderInfo::AddShaderInfo(info); \
}
#define DECLARE_SHADER_CS(Name, CS) DECLARE_SHADER_CS_WITH_OPTION(Name, CS, false, false);


struct jShader : public std::enable_shared_from_this<jShader>
{
	static std::unordered_map<size_t, std::shared_ptr<jShader> > ShaderMap;
	static std::vector<std::shared_ptr<jShader> > ShaderVector;
	static std::unordered_map<std::string, std::shared_ptr<jShader> > ShaderNameMap;
	static jShader* GetShader(size_t hashCode);
	static jShader* GetShader(const std::string& name);
	static jShader* CreateShader(const jShaderInfo& shaderInfo);
	static std::shared_ptr<jShader> GetShaderPtr(size_t hashCode);
	static std::shared_ptr<jShader> GetShaderPtr(const std::string& name);
	static std::shared_ptr<jShader> CreateShaderPtr(const jShaderInfo& shaderInfo);
	static void UpdateShaders();

	bool SetUniformbuffer(const char* name, const Matrix& InData) const;
	bool SetUniformbuffer(const char* name, const bool InData) const;
	bool SetUniformbuffer(const char* name, const float InData) const;
	bool SetUniformbuffer(const char* name, const int32 InData) const;
	bool SetUniformbuffer(const char* name, const Vector2& InData) const;
	bool SetUniformbuffer(const char* name, const Vector& InData) const;
	bool SetUniformbuffer(const char* name, const Vector4& InData) const;

	uint64 TimeStamp = 0;
	jShaderInfo ShaderInfo;

	void UpdateShader();
};