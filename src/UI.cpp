#include "DataHandler.h"
#include "UI.h"
#include "utils.h"

SMFRenderer* SMFRenderer::GetSingleton()
{
	static SMFRenderer Singleton;
	return std::addressof(Singleton);
}

void SMFRenderer::Register()
{
	if (!SKSEMenuFramework::IsInstalled()) {
		logger::error("Unable to Register For SKSEMenuFramework, Please install SKSEMenuFramework to configure the jsons");
		return;
	}

	SMFRenderer* s = GetSingleton();

	std::lock_guard<std::mutex> lock(s->_lock);

	s->GetAllExclusionJsons();
	SKSEMenuFramework::SetSection("Ammo Patcher");

	std::ifstream FileContainingHintsForMain("Data/SKSE/Plugins/Ammo Patcher Hints/MainHint.json");
	s->_HintsForMain = nJson::parse(FileContainingHintsForMain);

	SKSEMenuFramework::AddSectionItem("Main", SMFRenderer::RenderMain);

	std::ifstream FileContainingHintsForExclusions("Data/SKSE/Plugins/Ammo Patcher Hints/ExclusionHint.json");
	s->_HintsForExclusions = nJson::parse(FileContainingHintsForExclusions);

	s->_LogWindow = SKSEMenuFramework::AddWindow(s->RenderLogWindow);

	SKSEMenuFramework::AddSectionItem("Exclusions", SMFRenderer::RenderExclusions);
	logger::info("Registered For SKSEMenuFramework");
}

inline void __stdcall SMFRenderer::RenderLogWindow()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	static std::string    name("Log Window##");
	static std::once_flag o;
	std::call_once(o, []() {
		name += SKSE::PluginDeclaration::GetSingleton()->GetName();
	});

	ImGui::Begin(name.c_str(), nullptr, ImGuiWindowFlags_None);

	CustomLogger* c = CustomLogger::GetSingleton();

	static char filterInput[128] = "";
	ImGui::InputText("Filter", filterInput, ((int)(sizeof(filterInput) / sizeof(*filterInput))));

	if (ImGui::Button("Clear Logs")) {
		c->ClearLogs();
	}
	ImGui::SameLine();
	if (ImGui::Button("Close Window")) {
		_LogWindow->IsOpen = false;
		c->AddLog("User Closed Log Window");
	}

	ImGui::Separator();
	ImGui::BeginChild("LogScroll");
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));

	auto logs = filterInput[0] ? c->GetFilteredLogs(filterInput) : c->GetLogs();
	for (size_t i = 0; i < logs.size(); i++) {
		ImGui::TextUnformatted(std::format("[{}] {}", i + 1sz, logs[i]).c_str());
	}

	if (c->ShouldScrollToBottom()) {
		ImGui::SetScrollHereY(1.0f);
		c->ResetScrollToBottom();
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::End();
}

void SMFRenderer::RenderMain()
{
	SMFRenderer*                s = GetSingleton();

	std::lock_guard<std::mutex> lock(s->_lock);
	std::string_view            CurrentPath{ "Main."sv };

	ImGui::PushID(CurrentPath.data());

	DataHandler*                 d = DataHandler::GetSingleton();
	{
		std::lock_guard<std::mutex> dlock(d->_lock);

		s->RenderJsonEditor(CurrentPath, d->_JsonData, s->_HintsForMain);
	}

	if (ImGui::Button("Save JSON")) {
		std::string MainFileName(std::format("Data/SKSE/Plugins/{}.json", SKSE::PluginDeclaration::GetSingleton()->GetName()));
		CustomLogger*               c = CustomLogger::GetSingleton();
		if (s->SaveJsonToFile(MainFileName, d->_JsonData))
			c->AddLog(std::format("User Saved {} Successfully", MainFileName));
		else
			c->AddLog(std::format("Failed to Save {}", MainFileName));
	}
	

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("You Must Click This to SAVE any Changes");
	}
	static ImVec2 LoggingButtonSize;
	ImGui::GetContentRegionAvail(std::addressof(LoggingButtonSize));
	LoggingButtonSize.y = 0;
	if (ImGui::Button(CustomLogger::GetSingleton()->GetLatestLog().c_str(), LoggingButtonSize)) {
		_LogWindow->IsOpen = true;
	}

	ImGui::PopID();
}

void SMFRenderer::RenderExclusions()
{
	SMFRenderer*                s = GetSingleton();
	CustomLogger*               c = CustomLogger::GetSingleton();
	std::unique_lock<std::mutex> SMFLock(s->_lock);

	if (!s->_ExclusionJsons.empty()) {
		size_t ExclusionJsonsSize{ s->_ExclusionJsons.size() };
		for (size_t index = 0sz; index < ExclusionJsonsSize; index++) {
			std::string            name(s->_ExclusionJsons[index].first);
			std::shared_ptr<nJson> ej{ s->_ExclusionJsons[index].second };
			static const char*     key1{ "AMMO FormID to Exclude" };
			static const char*     key2{ "Mod File(s) to Exclude" };
			std::string            path1(name + "." + key1);
			std::string            path2(name + "." + key2);
			nJson&                 AMMOFormIDToExclude{ ej->at(key1) };
			nJson&                 ModFilesToExclude{ ej->at(key2) };
			if (ImGui::TreeNode(fs::path(name).filename().string().c_str())) {
				ImGui::PushID(path1.c_str());
				if (ImGui::TreeNode(key1)) {
					s->RenderJsonEditor(path1, AMMOFormIDToExclude, s->_HintsForExclusions);
					DataHandler*                d = DataHandler::GetSingleton();
					std::lock_guard<std::mutex> dlock(d->_lock);
					if (d->_Done) {
						static std::vector<std::string> a_value_for_FormID(ExclusionJsonsSize, d->_AmmoInfo[0]["AmmoString"].get<std::string>());  //used to store the selected value of _AmmoInfo of the currently traversed json
						static std::vector<size_t>      CurrentIterationForFormID(ExclusionJsonsSize, 0sz);                                        //used to store the selected index of _AmmoInfo of the currently traversed json

						if (a_value_for_FormID.size() != ExclusionJsonsSize) {
							a_value_for_FormID.resize(ExclusionJsonsSize, d->_AmmoInfo[0]["AmmoString"].get<std::string>());
						}
						if (CurrentIterationForFormID.size() != ExclusionJsonsSize) {
							CurrentIterationForFormID.resize(ExclusionJsonsSize, 0sz);
						}

						InlineUtils::limit(CurrentIterationForFormID[index], 0sz, d->_AmmoInfo.size());

						if (ImGui::BeginCombo(key1, d->_AmmoInfo[CurrentIterationForFormID[index]]["AmmoString"].get<std::string>().c_str())) {
							for (size_t ii = 0; ii < d->_AmmoInfo.size(); ii++) {
								bool isSelected = (CurrentIterationForFormID[index] == ii);
								if (ImGui::Selectable(d->_AmmoInfo[ii]["AmmoString"].get<std::string>().c_str(), isSelected)) {
									CurrentIterationForFormID[index] = ii;
									a_value_for_FormID[index] = d->_AmmoInfo[CurrentIterationForFormID[index]]["AmmoString"].get<std::string>();
								}
								if (isSelected) {
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}
						ImGui::SameLine();
						if (ImGui::Button(UI::plus.c_str())) {
							AMMOFormIDToExclude.push_back(a_value_for_FormID[index].c_str());
							c->AddLog(std::format("User Added '{}' into '{}' in the section {}", a_value_for_FormID[index], name, key1));
							ImGui::TreePop();
							ImGui::PopID();
							ImGui::TreePop();
							return;
						}
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode(key2)) {
					s->RenderJsonEditor(path1, ModFilesToExclude, s->_HintsForExclusions);
					DataHandler*                d = DataHandler::GetSingleton();
					std::lock_guard<std::mutex> ddlock(d->_lock);
					if (d->_Done) {
						static std::vector<std::string> a_value(ExclusionJsonsSize, d->_AmmoModFiles[0]);
						static std::vector<size_t>      CurrentIteration(ExclusionJsonsSize, 0sz);

						if (a_value.size() != ExclusionJsonsSize) {
							a_value.resize(ExclusionJsonsSize, d->_AmmoModFiles[0]);
						}
						if (CurrentIteration.size() != ExclusionJsonsSize) {
							CurrentIteration.resize(ExclusionJsonsSize, 0sz);
						}

						InlineUtils::limit(CurrentIteration[index], 0sz, d->_AmmoModFiles.size());

						if (ImGui::BeginCombo(key2, d->_AmmoModFiles[CurrentIteration[index]].c_str())) {
							for (size_t iii = 0; iii < d->_AmmoModFiles.size(); iii++) {
								bool isSelected = (CurrentIteration[index] == iii);
								if (ImGui::Selectable(d->_AmmoModFiles[iii].c_str(), isSelected)) {
									CurrentIteration[index] = iii;
									a_value[index] = d->_AmmoModFiles[CurrentIteration[index]];
								}
								if (isSelected) {
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}
						ImGui::SameLine();
						if (ImGui::Button(UI::plus.c_str())) {
							ModFilesToExclude.push_back(a_value[index].c_str());
							c->AddLog(std::format("User Added '{}' into '{}' in the section {}", a_value[index], name, key2));
							ImGui::TreePop();
							ImGui::PopID();
							ImGui::TreePop();
							return;
						}
					}
					ImGui::TreePop();
				}

				
				if (ImGui::Button("Save JSON")) {
					if (s->SaveJsonToFile(name, ej))
						c->AddLog(std::format("User Saved {} Successfully", name));
					else
						c->AddLog(std::format("Failed to Save {}", name));
				}
				ImGui::SameLine();
				if (ImGui::Button("Delete File")) {
					if (fs::remove(name)) {
						ej.reset();
						s->_ExclusionJsons.erase(s->_ExclusionJsons.begin() + index);
						c->AddLog(std::format("User Deleted {} Successfully", name));

						ImGui::PopID();
						ImGui::TreePop();
						return;
					} else {
						c->AddLog(std::format("Failed to Delete {}", name));
					}
				}
				ImGui::PopID();
				ImGui::TreePop();
			}

		}
	}
	static char buffer[256] = "";
	if (ImGui::InputText("Create a Exclusion File?", buffer, ((int)(sizeof(buffer) / sizeof(*buffer))), ImGuiInputTextFlags_EnterReturnsTrue)) {
		std::string     FileName = std::format("Data/SKSE/Plugins/Ammo Patcher/{}.json", buffer);
		std::shared_ptr j = std::make_shared<nJson>(nJson{ { "AMMO FormID to Exclude", nJson::array() }, { "Mod File(s) to Exclude", nJson::array() } });

		switch (s->CreateNewJsonFile(FileName, j)) {
		case FileCreationType::OK:
			s->_ExclusionJsons.push_back({ FileName, j });
			c->AddLog(std::format("User Created {} Successfully", FileName));
			break;
		case FileCreationType::Error:
			c->AddLog(std::format("Error in Creating File {}", FileName));
			j.reset();
			break;
		case FileCreationType::Duplicate:
			c->AddLog(std::format("Not Creating Duplicate File {}", FileName));
			j.reset();
			break;
		}
	}
	SMFLock.unlock();
	ImGui::SameLine();
	ImGui::Button(UI::question.c_str());

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Enter a Name with less than 256 characters.\nThe File will be created in the Appropriate Path with the Appropriate Extension");
	}
	static ImVec2 LoggingButtonSize;
	ImGui::GetContentRegionAvail(std::addressof(LoggingButtonSize));
	LoggingButtonSize.y = 0;
	if (ImGui::Button(CustomLogger::GetSingleton()->GetLatestLog().c_str(), LoggingButtonSize)) {
		_LogWindow->IsOpen = true;
	}
}

void SMFRenderer::GetAllExclusionJsons()
{
	DataHandler* d = DataHandler::GetSingleton();

	std::lock_guard<std::mutex> lock(d->_lock);
	if (fs::exists(d->_FolderPath) && !fs::is_empty(d->_FolderPath)) {
		for (const fs::directory_entry& entry : fs::directory_iterator(d->_FolderPath)) {
			if (entry.path().extension() == ".json") {
				std::ifstream jFile(entry.path());
				try {
					std::shared_ptr<nJson> MergeData = std::make_shared<nJson>(nJson::parse(jFile));
					_ExclusionJsons.push_back({ entry.path().string(), MergeData });
				} catch (const nJson::parse_error& e) {
					logger::error("{}", e.what());
				}
			}
		}
	}
}

void SMFRenderer::RenderJsonEditor(const std::string_view Path, nJson& jsonObject, nJson& hint)
{
	if (jsonObject.is_object()){
		RenderJsonObject(Path, jsonObject, hint);
	}
	else if (jsonObject.is_array()){
		RenderJsonArray(Path, "", jsonObject, hint);
	} else {
		logger::error("Wierd Error Detected at SMFRenderer::RenderJsonEditor. Given json object is neither object nor array");
		CustomLogger::GetSingleton()->AddLog("[error] Wierd Error Detected at SMFRenderer::RenderJsonEditor. Given json object is neither object nor array");
	}
}

void SMFRenderer::RenderJsonValue(const std::string_view jsonPath, const std::string& key, nJson& value, nJson& hint)
{
	CustomLogger* c = CustomLogger::GetSingleton();
	std::string   currentPath(jsonPath);
	currentPath += key + ".";
	ImGui::PushID(currentPath.c_str());  // Using jsonPath as unique ID for ImGui

	if (value.is_object()) {
		// Render object
		if (ImGui::TreeNode((key + " { }").c_str())) {
			RenderJsonObject(currentPath, value, hint);
			ImGui::TreePop();
		}
	} else if (value.is_array()) {
		// Render array
		if (ImGui::TreeNode((key + " [ ]").c_str())) {
			RenderJsonArray(currentPath, key, value, hint);
			ImGui::TreePop();
		}
	} else if (value.is_string()) {
		// Render string input, combo boxes, etc.
		if (key == "LogLevel") {
			static int         currentLogLevel = 2;
			static const char* LogLevels[6] = { "trace", "debug", "info", "warn", "err", "critical" };
			if (ImGui::Combo(key.c_str(), &currentLogLevel, LogLevels, ((int)(sizeof(LogLevels) / sizeof(*LogLevels))))) {
				value = std::string(LogLevels[currentLogLevel]);
				c->AddLog(std::format("User Selected LogLevel of key '{}' in Path {} to {}", key, jsonPath, LogLevels[currentLogLevel]));
			}
		} else if (key == "Sound Level") {
			enum AmmoType
			{
				Arrow,
				Bolt,
				None
			};
			AmmoType a = AmmoType::None;
			if (currentPath.find(".Arrow.") != std::string::npos) {
				a = AmmoType::Arrow;
			} else if (currentPath.find(".Bolt.") != std::string::npos) {
				a = AmmoType::Bolt;
			}
			static int         CurrentSoundLevel[2] = { 2, 2 };
			static const char* SoundLevel[5] = { "kLoud", "kNormal", "kSilent", "kVeryLoud", "kQuiet" };
			switch (a) {
			case AmmoType::Arrow:
			case AmmoType::Bolt:
				if (ImGui::Combo(key.c_str(), &CurrentSoundLevel[a], SoundLevel, ((int)(sizeof(SoundLevel) / sizeof(*SoundLevel))))) {
					value = std::string(SoundLevel[CurrentSoundLevel[a]]);
					c->AddLog(std::format("User Selected SoundLevel  of key '{}' in Path {} to {}", key, jsonPath, SoundLevel[CurrentSoundLevel[a]]));
				}
				break;
			default:
				break;
			}
		} else {
			char buffer[256];
			strncpy(buffer, value.get<std::string>().c_str(), sizeof(buffer));
			buffer[sizeof(buffer) - 1] = 0;
			if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
				value = std::string(buffer);
				c->AddLog(std::format("User Changed Value of key '{}' in Path {} to {}", key, jsonPath, buffer));
			}
		}
	} else if (value.is_boolean()) {
		// Render checkbox for boolean
		bool boolValue = value.get<bool>();
		if (ImGui::Checkbox(key.c_str(), &boolValue)) {
			value = boolValue;
			c->AddLog(std::format("User Changed Value of Key '{}' in Path {} to {}", key, jsonPath, boolValue));
		}
	} else if (value.is_number_integer()) {
		// Render input for integer
		int intValue = value.get<int>();
		if (ImGui::InputInt(key.c_str(), &intValue)) {
			value = intValue;
			c->AddLog(std::format("User Changed Value of Key '{}' in Path {} to {}", key, jsonPath, intValue));
		}
	} else if (value.is_number_float()) {
		// Render input for float
		float floatValue = value.get<float>();
		if (ImGui::InputFloat(key.c_str(), &floatValue)) {
			value = floatValue;
			c->AddLog(std::format("User Changed Value of Key '{}' in Path {} to {}", key, jsonPath, floatValue));
		}
	}

	RenderHint(hint);
	ImGui::PopID();
}

void SMFRenderer::RenderJsonObject(const std::string_view jsonPath, nJson& j, nJson& hint)
{
	for (auto& [key, value] : j.items()) {
		std::string currentPath(jsonPath);
		currentPath += key + ".";
		RenderJsonValue(currentPath, key, value, hint[key]);
	}
}

void SMFRenderer::RenderHint(nJson& hint)
{
	std::string clue = hint.is_string() ? hint.get<std::string>() : "";
	if (!clue.empty()) {
		ImGui::SameLine();
		if (ImGui::Button(UI::question.c_str())) {}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(clue.c_str());
		}
	}
}

SMFRenderer::FileCreationType SMFRenderer::CreateNewJsonFile(const std::string_view filename, const nJson& jsonObject)
{
	if (!fs::exists(filename)) {
		std::ofstream file(filename.data());

		if (file.is_open()) {
			file << jsonObject.dump();  // no need to Dump JSON with indentation of 4 spaces because user will(should) not manually edit json
			file.close();
			return FileCreationType::OK;
		} else {
			logger::error("Unable to open file for writing: {}", filename);
			return FileCreationType::Error;
		}
	} else {
		logger::error("Don't try to create Duplicates of : {}", filename);
		return FileCreationType::Duplicate;
	}
}

SMFRenderer::FileCreationType SMFRenderer::CreateNewJsonFile(const std::string_view filename, std::shared_ptr<nJson> jsonObject)
{
	if (!fs::exists(filename)) {
		std::ofstream file(filename.data());

		if (file.is_open()) {
			file << jsonObject->dump();  // no need to Dump JSON with indentation of 4 spaces because user will(should) not manually edit json
			file.close();
			return FileCreationType::OK;
		} else {
			logger::error("Unable to open file for writing: {}", filename);
			return FileCreationType::Error;
		}
	} else {
		logger::error("Don't try to create Duplicates of : {}", filename);
		return FileCreationType::Duplicate;
	}
}

bool SMFRenderer::SaveJsonToFile(const std::string_view filename, const nJson& jsonObject)
{
	std::ofstream file(filename.data());

	if (file.is_open()) {
		file << jsonObject.dump();  // no need to Dump JSON with indentation of 4 spaces because user will(should) not manually edit json
		file.close();
		return true;
	} else {
		logger::error("Unable to open file for writing: {}", filename);
	}
	return false;
}

bool SMFRenderer::SaveJsonToFile(const std::string_view filename, std::shared_ptr<nJson> jsonObject)
{
	std::ofstream file(filename.data());

	if (file.is_open()) {
		file << jsonObject->dump();  // no need to Dump JSON with indentation of 4 spaces because user will(should) not manually edit json
		file.close();
		return true;
	} else {
		logger::error("Unable to open file for writing: {}", filename);
	}
	return false;
}

void SMFRenderer::RenderJsonArray(const std::string_view jsonPath, std::string_view key, nJson& j, nJson& hint)
{
	CustomLogger* c = CustomLogger::GetSingleton();
	std::string   currentPath(jsonPath);
	currentPath += key;
	currentPath += ".";
	ImGui::PushID(currentPath.c_str());
	for (size_t i = 0; i < j.size(); ++i) {
		std::string Path(currentPath);
		Path += "[" + std::to_string(i) + "].";

		ImGui::PushID(Path.c_str());
		if (hint.is_array()) {
			RenderJsonValue(Path, "[" + std::to_string(i) + "]", j[i], hint[i]);
		} else {
			RenderJsonValue(Path, "[" + std::to_string(i) + "]", j[i], hint);
		}
		ImGui::SameLine();
		if (ImGui::Button(UI::x.c_str())) {
			c->AddLog(std::format("User Decided to Remove {} Value in Key '{}' in Path {}", j[i].dump(), key, jsonPath));
			j.erase(j.begin() + i);
			ImGui::PopID();
			ImGui::PopID();
			return;  // exit to avoid further processing as the array is modified
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(std::string("Removes line [" + std::to_string(i) + "]").c_str());
		}
		if (i > 0sz) {
			ImGui::SameLine();
			if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {  //Move up
				c->AddLog(std::format("User Decided to move {} Value in Key '{}' in Path {} One Step Above", j[i].dump(), key, jsonPath));
				std::swap(j[i], j[i - 1sz]);
				ImGui::PopID();
				ImGui::PopID();
				return;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Click Me to Move me Up");
			}
		}
		if (i < (j.size() - 1sz)) {
			ImGui::SameLine();
			if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {  //Move Down
				c->AddLog(std::format("User Decided to move {} Value from Key '{}' in Path {} One Step Below", j[i].dump(), key, jsonPath));
				std::swap(j[i], j[i + 1sz]);
				ImGui::PopID();
				ImGui::PopID();
				return;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Click Me to Move me Down");
			}
		}
		ImGui::PopID();
	}
	
	if (ImGui::Button(UI::plus.c_str())) {
		// add a default value to the array
		j.push_back("");
		c->AddLog(std::format("User Added Value \"\" into Last position of Key '{}' in Path {}", key, jsonPath));
		ImGui::PopID();
		ImGui::PopID();
		return;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Adds a Empty Line");
	}
	ImGui::PopID();
}

inline CustomLogger* CustomLogger::GetSingleton()
{
	static CustomLogger Singleton;
	return std::addressof(Singleton);
}

void CustomLogger::AddLog(const std::string& message)
{
	_logs.push_back(message);

	size_t oldSize = _lineOffsets.empty() ? 0 : _lineOffsets.back();
	for (char c : message) {
		oldSize++;
		if (c == '\n') {
			_lineOffsets.push_back(oldSize);
		}
	}
	_scrollToBottom = true;
}

inline void CustomLogger::ClearLogs()
{
	std::lock_guard<std::mutex> lock(_lock);
	_logs.clear();
	_lineOffsets.clear();
	_scrollToBottom = false;
}

inline const std::vector<std::string>& CustomLogger::GetLogs() const
{
	std::lock_guard<std::mutex> lock(_lock);
	return _logs;
}

inline const std::string CustomLogger::GetLatestLog() const
{
	std::lock_guard<std::mutex> lock(_lock);

	static const std::string emptyLog = "Default(No Logs available)";
	if (_logs.empty()) {
		return emptyLog;
	}

	std::string formattedLog = std::format("[{}] {}", _logs.size(), _logs.back());
	return formattedLog;
	
}

bool CustomLogger::ShouldScrollToBottom() const
{
	std::lock_guard<std::mutex> lock(_lock);
	return _scrollToBottom;
}

void CustomLogger::ResetScrollToBottom()
{
	std::lock_guard<std::mutex> lock(_lock);
	_scrollToBottom = false;
}

std::vector<std::string> CustomLogger::GetFilteredLogs(const std::string& filter) const
{
	std::lock_guard<std::mutex> lock(_lock);
	std::vector<std::string>    filteredLogs;
	for (const auto& log : _logs) {
		if (log.find(filter) != std::string::npos) {
			filteredLogs.push_back(log);
		}
	}
	return filteredLogs;
}
