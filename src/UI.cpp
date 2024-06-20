#include "DataHandler.h"
#include "UI.h"
#include "utils.h"

SMFRenderer* SMFRenderer::GetSingleton()
{
	static SMFRenderer Singleton;
	return std::addressof(Singleton);
}

void __stdcall SMFRenderer::Register()
{
	if (!SKSEMenuFramework::IsInstalled()) {
		logger::error("Unable to Register For SKSEMenuFramework, Please install SKSEMenuFramework to configure the jsons");
		return;
	}

	SMFRenderer*                s = SMFRenderer::GetSingleton();
	std::lock_guard<std::mutex> lock(s->_lock);

	s->GetAllExclusionJsons();
	SKSEMenuFramework::SetSection("Ammo Patcher");

	std::ifstream FileContainingHintsForMain("Data/SKSE/Plugins/Ammo Patcher Hints/MainHint.json");
	s->_HintsForMain = nJson::parse(FileContainingHintsForMain);

	SKSEMenuFramework::AddSectionItem("Main", SMFRenderer::RenderMain);

	std::ifstream FileContainingHintsForExclusions("Data/SKSE/Plugins/Ammo Patcher Hints/ExclusionHint.json");
	s->_HintsForExclusions = nJson::parse(FileContainingHintsForExclusions);

	SKSEMenuFramework::AddSectionItem("Exclusions", SMFRenderer::RenderExclusions);
	logger::info("Registered For SKSEMenuFramework");
}

void __stdcall SMFRenderer::RenderMain()
{
	SMFRenderer*                s = GetSingleton();
	std::lock_guard<std::mutex> lock(s->_lock);

	std::string_view CurrentPath{ "Main"sv };

	ImGui::PushID(CurrentPath.data());

	DataHandler*                 d = DataHandler::GetSingleton();
	std::unique_lock<std::mutex> dlock(d->_lock);

	s->RenderJsonEditor(CurrentPath, d->_JsonData, s->_HintsForMain);

	if (ImGui::Button("Save JSON")) {
		//Ignore [[ nodiscard ]] warning here
		s->SaveJsonToFile(std::format("Data/SKSE/Plugins/{}.json", SKSE::PluginDeclaration::GetSingleton()->GetName()), d->_JsonData);
	}
	dlock.unlock();

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("You Must Click This to SAVE any Changes");
	}

	ImGui::PopID();
}

void __stdcall SMFRenderer::RenderExclusions()
{
	SMFRenderer*                s = GetSingleton();
	std::lock_guard<std::mutex> lock(s->_lock);

	if (!s->_ExclusionJsons.empty()) {
		for (size_t index = 0sz; index < s->_ExclusionJsons.size(); index++) {
			ImGui::PushID(s->_ExclusionJsons[index].first.c_str());

			if (ImGui::TreeNode(fs::path(s->_ExclusionJsons[index].first).filename().string().c_str())) {
				for (auto& [key, value] : s->_ExclusionJsons[index].second->items()) {
					std::string currentPath(s->_ExclusionJsons[index].first);
					currentPath += "." + key + ".";
					if (ImGui::TreeNode(key.c_str())) {
						s->RenderJsonArray(currentPath, value, s->_HintsForExclusions);
						ImGui::TreePop();
					}
					if (key == "AMMO FormID to Exclude") {
						DataHandler*                d = DataHandler::GetSingleton();
						std::lock_guard<std::mutex> dlock(d->_lock);
						if (d->_Done) {
							static std::vector<std::string> a_value_for_FormID(s->_ExclusionJsons.size(), d->_AmmoInfo[0]["AmmoString"].get<std::string>());  //used to store the selected value of _AmmoInfo of the currently traversed json
							static std::vector<size_t>      CurrentIterationForFormID(s->_ExclusionJsons.size(), 0sz);  //used to store the selected index of _AmmoInfo of the currently traversed json

							if (a_value_for_FormID.size() != s->_ExclusionJsons.size()) {
								a_value_for_FormID.resize(s->_ExclusionJsons.size(), d->_AmmoInfo[0]["AmmoString"].get<std::string>());
							}
							if (CurrentIterationForFormID.size() != s->_ExclusionJsons.size()) {
								CurrentIterationForFormID.resize(s->_ExclusionJsons.size(), 0sz);
							}

							InlineUtils::limit(CurrentIterationForFormID[index], 0sz, d->_AmmoInfo.size());

							if (ImGui::BeginCombo(key.c_str(), d->_AmmoInfo[CurrentIterationForFormID[index]]["AmmoString"].get<std::string>().c_str())) {
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
							if (ImGui::Button(s->_plus.c_str())) {
								value.push_back(a_value_for_FormID[index].c_str());
								ImGui::PopID();
								return;
							}
						}
					} else if (key == "Mod File(s) to Exclude") {
						DataHandler*                d = DataHandler::GetSingleton();
						std::lock_guard<std::mutex> ddlock(d->_lock);
						if (d->_Done) {
							static std::vector<std::string> a_value(s->_ExclusionJsons.size(), d->_AmmoModFiles[0]);
							static std::vector<size_t>      CurrentIteration(s->_ExclusionJsons.size(), 0sz);

							if (a_value.size() != s->_ExclusionJsons.size()) {
								a_value.resize(s->_ExclusionJsons.size(), d->_AmmoModFiles[0]);
							}
							if (CurrentIteration.size() != s->_ExclusionJsons.size()) {
								CurrentIteration.resize(s->_ExclusionJsons.size(), 0sz);
							}

							InlineUtils::limit(CurrentIteration[index], 0sz, d->_AmmoModFiles.size());

							if (ImGui::BeginCombo(key.c_str(), d->_AmmoModFiles[CurrentIteration[index]].c_str())) {
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
							if (ImGui::Button(s->_plus.c_str())) {
								value.push_back(a_value[index].c_str());
								ImGui::PopID();
								return;
							}
						}
					}
				}

				if (ImGui::Button("Save JSON")) {
					//Ignore [[ nodiscard ]] warning here
					s->SaveJsonToFile(s->_ExclusionJsons[index].first, *s->_ExclusionJsons[index].second);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("You Must Click This to SAVE any Changes");
				}
				ImGui::SameLine();
				if (ImGui::Button(s->_x.c_str())) {
					try {
						fs::remove(s->_ExclusionJsons[index].first);
						delete s->_ExclusionJsons[index].second;
					} catch (const fs::filesystem_error& e) {
						logger::error("Error Deleting File: {}", e.what());
					}
					s->_ExclusionJsons.erase(s->_ExclusionJsons.begin() + index);
					ImGui::TreePop();
					ImGui::PopID();
					return;  // exit to avoid further processing as the array is modified
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Click this to Delete File");
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	static char buffer[256] = "";
	if (ImGui::InputText("Create a Exclusion File?", buffer, ((int)(sizeof(buffer) / sizeof(*buffer))), ImGuiInputTextFlags_EnterReturnsTrue)) {
		std::string FileName = std::format("Data/SKSE/Plugins/Ammo Patcher/{}.json", buffer);
		nJson*      j = new nJson();

		*j = { { "AMMO FormID to Exclude", nJson::array() }, { "Mod File(s) to Exclude", nJson::array() } };
		if (s->SaveJsonToFile(FileName, *j)) {
			s->_ExclusionJsons.push_back({ FileName, j });
		} else {
			delete j;
		}
	}
	ImGui::SameLine();
	ImGui::Button(s->_question.c_str());

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Enter a Name with less than 256 characters.\nThe File will be created in the Appropriate Path with the Appropriate Extension");
	}
}

void SMFRenderer::GetAllExclusionJsons()
{
	DataHandler* d = DataHandler::GetSingleton();

	if (fs::exists(d->_FolderPath) && !fs::is_empty(d->_FolderPath)) {
		for (const fs::directory_entry& entry : fs::directory_iterator(d->_FolderPath)) {
			if (entry.path().extension() == ".json") {
				std::ifstream jFile(entry.path());
				nJson*        MergeData = new nJson();
				try {
					*MergeData = nJson::parse(jFile);
					_ExclusionJsons.push_back({ entry.path().string(), MergeData });
				} catch (const nJson::parse_error& e) {
					logger::error("{}", e.what());
				}
			}
		}
	}
}

void SMFRenderer::RenderJsonValue(const std::string_view jsonPath, const std::string& key, nJson& value, nJson& hint)
{
	std::string currentPath(jsonPath);
	currentPath += key + ".";
	ImGui::PushID(currentPath.c_str());  // Using jsonPath as unique ID for ImGui

	if (value.is_object()) {
		// Render object
		if (ImGui::TreeNode((key + " { }").c_str())) {
			SMFRenderer::RenderJsonObject(currentPath, value, hint);
			ImGui::TreePop();
		}
	} else if (value.is_array()) {
		// Render array
		if (ImGui::TreeNode((key + " [ ]").c_str())) {
			SMFRenderer::RenderJsonArray(currentPath,  value, hint);
			ImGui::TreePop();
		}
	} else if (value.is_string()) {  //Fix This
		// Render string input, combo boxes, etc.
		if (key == "LogLevel") {
			static int         currentLogLevel = 2;
			static const char* LogLevels[] = { "trace", "debug", "info", "warn", "err", "critical" };
			if (ImGui::Combo(key.c_str(), &currentLogLevel, LogLevels, ((int)(sizeof(LogLevels) / sizeof(*LogLevels))))) {
				value = std::string(LogLevels[currentLogLevel]);
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
			static const char* SoundLevel[] = { "kLoud", "kNormal", "kSilent", "kVeryLoud", "kQuiet" };
			switch (a) {
			case AmmoType::Arrow:
			case AmmoType::Bolt:
				if (ImGui::Combo(key.c_str(), &CurrentSoundLevel[a], SoundLevel, ((int)(sizeof(SoundLevel) / sizeof(*SoundLevel))))) {
					value = std::string(SoundLevel[CurrentSoundLevel[a]]);
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
			}
		}
	} else if (value.is_boolean()) {
		// Render checkbox for boolean
		bool boolValue = value.get<bool>();
		if (ImGui::Checkbox(key.c_str(), &boolValue)) {
			value = boolValue;
		}
	} else if (value.is_number_integer()) {
		// Render input for integer
		int intValue = value.get<int>();
		if (ImGui::InputInt(key.c_str(), &intValue)) {
			value = intValue;
		}
	} else if (value.is_number_float()) {
		// Render input for float
		float floatValue = value.get<float>();
		if (ImGui::InputFloat(key.c_str(), &floatValue)) {
			value = floatValue;
		}
	}

	SMFRenderer::RenderHint(hint);
	ImGui::PopID();
}

void SMFRenderer::RenderHint(nJson& hint)
{
	std::string clue = hint.is_string() ? hint.get<std::string>() : "";
	if (!clue.empty()) {
		ImGui::SameLine();
		if (ImGui::Button(_question.c_str())) {}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(clue.c_str());
		}
	}
}

void SMFRenderer::RenderJsonObject(const std::string_view jsonPath, nJson& j, nJson& hint)
{
	for (auto& [key, value] : j.items()) {
		std::string currentPath(jsonPath);
		currentPath += key + ".";
		SMFRenderer::RenderJsonValue(currentPath, key, value, hint[key]);
	}
}

void SMFRenderer::RenderJsonArray(const std::string_view jsonPath, nJson& j, nJson& hint)
{
	for (size_t i = 0; i < j.size(); ++i) {
		std::string currentPath(jsonPath);
		currentPath += "[" + std::to_string(i) + "].";

		ImGui::PushID(currentPath.c_str());
		if (hint.is_array()) {
			SMFRenderer::RenderJsonValue(currentPath, "[" + std::to_string(i) + "]", j[i], hint[i]);
		} else {
			SMFRenderer::RenderJsonValue(currentPath, "[" + std::to_string(i) + "]", j[i], hint);
		}
		ImGui::SameLine();
		if (ImGui::Button(_x.c_str())) {
			j.erase(j.begin() + i);
			ImGui::PopID();
			return;  // exit to avoid further processing as the array is modified
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(std::string("Removes line [" + std::to_string(i) + "]").c_str());
		}
		ImGui::PopID();
	}

	if (ImGui::Button(_plus.c_str())) {
		// Example: add a default value to the array
		j.push_back("");
		return;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Adds a Empty Line");
	}
}

void SMFRenderer::RenderJsonEditor(const std::string_view Path, nJson& jsonObject, nJson& hint)
{
	std::string CurrentPath(Path);
	CurrentPath.append(".");
	SMFRenderer::RenderJsonObject(CurrentPath, jsonObject, hint);
}

[[nodiscard]] bool SMFRenderer::SaveJsonToFile(const std::string_view filename, const nJson& jsonObject)
{
	if (!fs::exists(filename)) {
		std::ofstream file(filename.data());

		if (file.is_open()) {
			file << jsonObject.dump();  // no need to Dump JSON with indentation of 4 spaces because user will(should) not manually edit json
			file.close();
			return true;
		} else {
			logger::error("Unable to open file for writing: {}", filename);
		}
	} else {
		logger::error("Don't try to create Duplicates of : {}", filename);
	}
	return false;
}
