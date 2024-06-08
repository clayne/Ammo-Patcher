#include "DataHandler.h"
#include "Events.h"
#include "logging.h"


SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	auto startSPL = std::chrono::high_resolution_clock::now();
	AP::InitializeLogging();

	DataHandler::GetSingleton()->LoadJson();
	SKSE::Init(a_skse);

	SKSEEvent::InitializeMessaging();

	auto nanosecondsTakenForSPL = std::chrono::duration(std::chrono::high_resolution_clock::now() - startSPL);

	logger::info("Time Taken in SKSEPluginLoad(const SKSE::LoadInterface* a_skse) totally is {} nanoseconds or {} microseconds or {} milliseconds or {} seconds or {} minutes", nanosecondsTakenForSPL.count(),
		std::chrono::duration_cast<std::chrono::microseconds>(nanosecondsTakenForSPL).count(), std::chrono::duration_cast<std::chrono::milliseconds>(nanosecondsTakenForSPL).count(),
		std::chrono::duration_cast<std::chrono::seconds>(nanosecondsTakenForSPL).count(), std::chrono::duration_cast<std::chrono::minutes>(nanosecondsTakenForSPL).count());
	return true;
}
