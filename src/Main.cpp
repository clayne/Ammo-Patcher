#include "Main.h"

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	/*while (!IsDebuggerPresent()) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}*/
	std::chrono::steady_clock::time_point startSPL = std::chrono::high_resolution_clock::now();

	Logging logger;

	SKSE::Init(a_skse);
	DataHandler* d = DataHandler::GetSingleton();
	d->LoadJson();

	std::unique_lock<std::mutex> lock(d->_lock);
	if (d->_InfinitePlayerAmmo || d->_InfiniteTeammateAmmo) {
		APEventProcessor::RegisterEvent();
	}
	lock.unlock();

	SKSEEvent::InitializeMessaging();

	SMFRenderer::Register();

	std::chrono::nanoseconds nanosecondsTakenForSPL = std::chrono::duration(std::chrono::high_resolution_clock::now() - startSPL);

	logger::info("Time Taken in SKSEPluginLoad(const SKSE::LoadInterface* a_skse) totally is {} nanoseconds or {} microseconds or {} milliseconds or {} seconds or {} minutes", nanosecondsTakenForSPL.count(),
		std::chrono::duration_cast<std::chrono::microseconds>(nanosecondsTakenForSPL).count(), std::chrono::duration_cast<std::chrono::milliseconds>(nanosecondsTakenForSPL).count(),
		std::chrono::duration_cast<std::chrono::seconds>(nanosecondsTakenForSPL).count(), std::chrono::duration_cast<std::chrono::minutes>(nanosecondsTakenForSPL).count());
	return true;
}
