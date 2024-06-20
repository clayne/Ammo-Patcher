#include "logging.h"

Logging::Logging()
{
	std::lock_guard<std::mutex> lock(_lock);
	std::optional<fs::path>     path = logger::log_directory();

	if (!path) {
		util::report_and_fail("Unable to lookup SKSE logs directory.");
	}

	*path /= std::format("{}.log", SKSE::PluginDeclaration::GetSingleton()->GetName());

	std::shared_ptr<spdlog::logger> log = std::make_shared<spdlog::logger>(SKSE::PluginDeclaration::GetSingleton()->GetName().data());

	SetupLog(path, log);
}

Logging::Logging(std::string_view Name)
{
	std::lock_guard<std::mutex> lock(_lock);
	std::optional<fs::path>     path = logger::log_directory();

	if (!path) {
		util::report_and_fail("Unable to lookup SKSE logs directory.");
	}

	*path /= std::format("{}.log", Name);

	std::shared_ptr<spdlog::logger> log = std::make_shared<spdlog::logger>(Name.data());

	SetupLog(path, log);
}

void Logging::SetupLog(std::optional<fs::path> path, std::shared_ptr<spdlog::logger>& log)
{
	if (IsDebuggerPresent()) {
		log->sinks().reserve(2);
		log->sinks().push_back(_MSVCSink);
	} else {
		log->sinks().reserve(1);
	}
	log->sinks().push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));

	log->set_level(_CommonLogLevel);
	log->flush_on(_CommonLogLevel);
	set_default_logger(std::move(log));
	spdlog::set_pattern(_Pattern);
}
