#include "logging.h"

void AP::InitializeLogging()
{
	auto path = logger::log_directory();

	if (!path) {
		util::report_and_fail("Unable to lookup SKSE logs directory.");
	}

	*path /= std::format("{}.log", SKSE::PluginDeclaration::GetSingleton()->GetName());
	
	auto log = std::make_shared<spdlog::logger>(SKSE::PluginDeclaration::GetSingleton()->GetName().data());

	if (IsDebuggerPresent()) {
		log->sinks().reserve(2);
		log->sinks().push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	} else {
		log->sinks().reserve(1);
	}
	log->sinks().push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
	
	const char* pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v";
	const auto& commonLogLevel{ spdlog::level::info }; //Type: spdlog::level::level_enum

	log->set_level(commonLogLevel);
	log->flush_on(commonLogLevel);
	set_default_logger(std::move(log));
	spdlog::set_pattern(pattern);
}
