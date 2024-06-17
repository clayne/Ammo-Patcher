#include "UI.h"
#include "DataHandler.h"

void UI::Register()
{
	if (!SKSEMenuFramework::IsInstalled()) {
		logger::error("Unable to Register For SKSEMenuFramework, Please install SKSEMenuFramework to configure the jsons");
		return;
	}
	UI::Exclusions::GetAllExclusionJsons();
	SKSEMenuFramework::SetSection("Ammo Patcher");
	SKSEMenuFramework::AddSectionItem("Main", UI::AP::Render);
	SKSEMenuFramework::AddSectionItem("Exclusions", UI::Exclusions::Render);
	logger::info("Registered For SKSEMenuFramework");
}

void __stdcall UI::AP::Render()
{
	DataHandler* d = DataHandler::GetSingleton();
	if (d->_Done) {
		jsoning::RenderJsonEditor(d->_JsonData);
	}
	if (ImGui::Button("Save JSON")) {
		jsoning::SaveJsonToFile(std::format("Data/SKSE/Plugins/{}.json", SKSE::PluginDeclaration::GetSingleton()->GetName()), d->_JsonData);
	}
}

void __stdcall UI::Exclusions::Render()
{
	if (!ExclusionJsons.empty()) {
		for (auto j : ExclusionJsons) {
			if (ImGui::TreeNode(fs::path(j.first).filename().string().c_str())) {
				jsoning::RenderJsonEditor(*j.second);
				if (ImGui::Button("Save JSON")) {
					jsoning::SaveJsonToFile(j.first, *j.second);
				}
				ImGui::TreePop();
			}
		}
	}
}

void UI::Exclusions::GetAllExclusionJsons()
{
	DataHandler* d = DataHandler::GetSingleton();
	if (fs::exists(d->_FolderPath) && !fs::is_empty(d->_FolderPath)) {
		for (const auto& entry : fs::directory_iterator(d->_FolderPath)) {
			if (entry.path().extension() == ".json") {
				std::ifstream jFile(entry.path());
				nJson*        MergeData = new nJson();
				try {
					*MergeData = nJson::parse(jFile);
					
					ExclusionJsons.push_back({ entry.path().string(), MergeData });
				} catch (const nJson::parse_error& e) {
					logger::error("{}", e.what());
				}
			}
		}
	}
}

void jsoning::RenderJsonValue(const std::string& key, nJson& value)
{
	ImGui::PushID(key.c_str());
	if (value.is_object()) {
		if (ImGui::TreeNode((key + " { }").c_str())) {
			RenderJsonObject(value);
			ImGui::TreePop();
		}
	} else if (value.is_array()) {
		if (ImGui::TreeNode((key + " [ ]").c_str())) {
			RenderJsonArray(value);
			ImGui::TreePop();
		}
	} else if (value.is_string()) {
		char buffer[256];
		strncpy(buffer, value.get<std::string>().c_str(), sizeof(buffer));
		buffer[sizeof(buffer) - 1] = 0;
		if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
			value = std::string(buffer);
		}
	} else if (value.is_boolean()) {
		bool boolValue = value.get<bool>();
		if (ImGui::Checkbox(key.c_str(), &boolValue)) {
			value = boolValue;
		}
	} else if (value.is_number_integer()) {
		int intValue = value.get<int>();
		if (ImGui::InputInt(key.c_str(), &intValue)) {
			value = intValue;
		}
	} else if (value.is_number_float()) {
		float floatValue = value.get<float>();
		if (ImGui::InputFloat(key.c_str(), &floatValue)) {
			value = floatValue;
		}
	}
	ImGui::PopID();
}

void jsoning::RenderJsonObject(nJson& j)
{
	for (auto& [key, value] : j.items()) {
		RenderJsonValue(key, value);
	}
}

void jsoning::RenderJsonArray(nJson& j)
{
	for (size_t i = 0; i < j.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		RenderJsonValue("[" + std::to_string(i) + "]", j[i]);
		ImGui::SameLine();
		if (ImGui::Button(x.c_str())) {
			j.erase(j.begin() + i);
			ImGui::PopID();
			return;  // exit to avoid further processing as the array is modified
		}
		ImGui::PopID();
	}
	if (ImGui::Button(plus.c_str())) {
		// Add a default value to the array. Adjust the type as needed.
		j.push_back("");
	}
}

void jsoning::RenderJsonEditor(nJson& jsonObject)
{
	RenderJsonObject(jsonObject);
}

void jsoning::SaveJsonToFile(const std::string& filename, const nJson& jsonObject)
{
	std::ofstream file(filename);
	if (file.is_open()) {
		file << jsonObject.dump(4);  // Dump JSON with indentation of 4 spaces
		file.close();
	} else {
		logger::error("Unable to open file for writing: {}", filename);
	}
}
