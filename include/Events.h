#pragma once

class APEventProcessor : public RE::BSTEventSink<RE::TESContainerChangedEvent>
{
public:
	static void RegisterEvent();

	static void UnregisterEvent();

private:
	APEventProcessor() = default;
	~APEventProcessor() = default;
	APEventProcessor(const APEventProcessor&) = delete;
	APEventProcessor(APEventProcessor&&) = delete;
	APEventProcessor& operator=(const APEventProcessor&) = delete;
	APEventProcessor& operator=(APEventProcessor&&) = delete;

	static APEventProcessor* GetSingleton();

	RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* e, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override;
};

class SKSEEvent
{
public:
	static void InitializeMessaging();

private:
	static void MessageListener(SKSE::MessagingInterface::Message* message);
};
