#pragma once
#include "SKSEMenuFramework.h"

class SMFRenderer
{
public:
	void static __stdcall Register();

private:
	SMFRenderer() = default;
	~SMFRenderer() = default;
	SMFRenderer(const SMFRenderer&) = delete;
	SMFRenderer(SMFRenderer&&) = delete;
	SMFRenderer& operator=(const SMFRenderer&) = delete;
	SMFRenderer& operator=(SMFRenderer&&) = delete;

	static SMFRenderer* GetSingleton();
	void static __stdcall RenderMain();
	void static __stdcall RenderExclusions();
	void GetAllExclusionJsons();
	void RenderJsonValue(const std::string_view jsonPath, const std::string& key, nJson& value, nJson& hint);
	void RenderHint(nJson& hint);
	void RenderJsonObject(const std::string_view jsonPath, nJson& j, nJson& hint);
	void RenderJsonArray(const std::string_view jsonPath, nJson& j, nJson& hint);
	void RenderJsonEditor(const std::string_view Path, nJson& jsonObject, nJson& hint);
	[[ nodiscard ]] bool SaveJsonToFile(const std::string_view filename, const nJson& jsonObject);

	nJson                                       _HintsForMain;
	nJson                                       _HintsForExclusions;
	std::set<std::string>                       _ModNames;
	std::vector<std::pair<std::string, nJson*>> _ExclusionJsons;
	const std::string                           _plus{ FontAwesome::UnicodeToUtf8(0x2b) };
	const std::string                           _x{ FontAwesome::UnicodeToUtf8(0x58) };
	const std::string                           _question{ FontAwesome::UnicodeToUtf8(0x3f) };
	std::mutex                                  _lock;
};
