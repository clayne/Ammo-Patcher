#pragma once
#include "SKSEMenuFramework/SKSEMenuFramework.h"

class CustomLogger
{
public:
	static CustomLogger*            GetSingleton();
	void                            AddLog(const std::string& message);
	void                            ClearLogs();
	const std::vector<std::string>& GetLogs() const;
	const std::string               GetLatestLog() const;
	bool                            ShouldScrollToBottom() const;
	void                            ResetScrollToBottom();
	std::vector<std::string>        GetFilteredLogs(const std::string& filter) const;

private:
	CustomLogger() = default;
	~CustomLogger() = default;
	CustomLogger(const CustomLogger&) = delete;
	CustomLogger(CustomLogger&&) = delete;
	CustomLogger& operator=(const CustomLogger&) = delete;
	CustomLogger& operator=(CustomLogger&&) = delete;

	std::vector<std::string> _logs;
	std::vector<size_t>      _lineOffsets;
	mutable std::mutex       _lock;
	mutable bool             _scrollToBottom{ false };
};

namespace UI
{
	inline const std::string plus{ FontAwesome::UnicodeToUtf8(0x2b) };
	inline const std::string x{ FontAwesome::UnicodeToUtf8(0x58) };
	inline const std::string question{ FontAwesome::UnicodeToUtf8(0x3f) };
}

class SMFRenderer
{
public:
	static void __stdcall Register();

private:
	enum FileCreationType
	{
		OK,
		Duplicate,
		Error
	};
	SMFRenderer() = default;
	~SMFRenderer() = default;
	SMFRenderer(const SMFRenderer&) = delete;
	SMFRenderer(SMFRenderer&&) = delete;
	SMFRenderer& operator=(const SMFRenderer&) = delete;
	SMFRenderer& operator=(SMFRenderer&&) = delete;

	inline static MENU_WINDOW _LogWindow;

	static SMFRenderer* GetSingleton();
	static void __stdcall RenderMain();
	static void __stdcall RenderExclusions();
	static void __stdcall RenderLogWindow();
	void                           GetAllExclusionJsons();
	void                           RenderJsonEditor(const std::string_view Path, nJson& jsonObject, nJson& hint);
	void                           RenderJsonValue(const std::string_view jsonPath, const std::string& key, nJson& value, nJson& hint);
	void                           RenderJsonObject(const std::string_view jsonPath, nJson& j, nJson& hint);
	void                           RenderJsonArray(const std::string_view jsonPath, std::string_view key, nJson& j, nJson& hint);
	void                           RenderHint(nJson& hint);
	[[nodiscard]] FileCreationType CreateNewJsonFile(const std::string_view filename, const nJson& jsonObject);
	[[nodiscard]] FileCreationType CreateNewJsonFile(const std::string_view filename, std::shared_ptr<nJson> jsonObject);
	[[nodiscard]] bool             SaveJsonToFile(const std::string_view filename, const nJson& jsonObject);
	[[nodiscard]] bool             SaveJsonToFile(const std::string_view filename, std::shared_ptr<nJson> jsonObject);

	nJson                                                       _HintsForMain;
	nJson                                                       _HintsForExclusions;
	std::set<std::string>                                       _ModNames;
	std::vector<std::pair<std::string, std::shared_ptr<nJson>>> _ExclusionJsons;
	std::mutex                                                  _lock;
};
