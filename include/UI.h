#pragma once
#include "SKSEMenuFramework.h"

namespace UI
{
	void Register();
	namespace AP
	{
		void __stdcall Render();
	}
	namespace Exclusions
	{
		static inline std::vector<std::pair<std::string, nJson*>> ExclusionJsons;
		void __stdcall Render();
		void GetAllExclusionJsons();
	};
}
namespace jsoning
{
	inline std::string plus = FontAwesome::UnicodeToUtf8(0x2b);
	inline std::string x = FontAwesome::UnicodeToUtf8(0x58);
	void RenderJsonValue(const std::string& key, nJson& value);
	void RenderJsonObject(nJson& j);
	void RenderJsonArray(nJson& j);
	void RenderJsonEditor(nJson& jsonObject);
	void SaveJsonToFile(const std::string& filename, const nJson& jsonObject);
}
