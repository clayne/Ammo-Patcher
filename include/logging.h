#pragma once

class Logging
{
public:
	Logging();

	Logging(std::string_view Name);

private:
	void                                         SetupLog(std::optional<fs::path> path, std::shared_ptr<spdlog::logger>& log);
	const char*                                  _Pattern{ "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v" };
	const spdlog::level::level_enum&             _CommonLogLevel{ spdlog::level::info };
	std::shared_ptr<spdlog::sinks::msvc_sink_mt> _MSVCSink{ std::make_shared<spdlog::sinks::msvc_sink_mt>() };
	std::mutex                                   _lock;
};
