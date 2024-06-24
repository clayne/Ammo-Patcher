#include "Events.h"
#include "DataHandler.h"

void SKSEEvent::InitializeMessaging()
{
	if (!SKSE::GetMessagingInterface()->RegisterListener(MessageListener))
		util::report_and_fail("Unable to register message listener.");
}

void SKSEEvent::MessageListener(SKSE::MessagingInterface::Message* message)
{
	//DataHandler* d = DataHandler::GetSingleton();
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		DataHandler::GetSingleton()->ammo_patch();
		break;
	default:
		break;
	}
}

void APEventProcessor::RegisterEvent()
{
	logger::debug("Registering APEventProcessor's RE::TESContainerChangedEvent");
	RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESContainerChangedEvent>(GetSingleton());
}

void APEventProcessor::UnregisterEvent()
{
	logger::debug("Unregistering APEventProcessor's RE::TESContainerChangedEvent");
	RE::ScriptEventSourceHolder::GetSingleton()->RemoveEventSink<RE::TESContainerChangedEvent>(GetSingleton());
}

APEventProcessor* APEventProcessor::GetSingleton()
{
	static APEventProcessor Singleton;
	return std::addressof(Singleton);
}

RE::BSEventNotifyControl APEventProcessor::ProcessEvent(const RE::TESContainerChangedEvent* e, RE::BSTEventSource<RE::TESContainerChangedEvent>*)
{
	// this event handles all object transfers between containers in the game
	// this can be deived into multiple base events: OnItemRemoved and OnItemAdded
	if (e && e->baseObj != 0 && e->itemCount != 0) {
		RE::TESObjectREFR*  oldCont = RE::TESForm::LookupByID<RE::TESObjectREFR>(e->oldContainer);
		RE::TESObjectREFR*  newCont = RE::TESForm::LookupByID<RE::TESObjectREFR>(e->newContainer);
		RE::TESBoundObject* baseObj = RE::TESForm::LookupByID<RE::TESBoundObject>(e->baseObj);
		RE::UI*             ui = RE::UI::GetSingleton();
		if (baseObj && oldCont && ui && newCont == nullptr) {
			RE::Actor*                  actor = oldCont->As<RE::Actor>();
			DataHandler*                d = DataHandler::GetSingleton();
			std::lock_guard<std::mutex> lock(d->_lock);
			if (actor) {
				if ((actor->IsPlayerRef() && d->_InfinitePlayerAmmo) || (actor->IsInFaction(RE::TESForm::LookupByID<RE::TESFaction>(0x0005C84E)) && d->_InfiniteTeammateAmmo)) {
					if (!(
							ui->IsMenuOpen(RE::GiftMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::FavoritesMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::LoadWaitSpinner::MENU_NAME) ||
							ui->IsMenuOpen(RE::TweenMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::SleepWaitMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::MagicMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::BookMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::JournalMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) ||
							ui->IsMenuOpen(RE::StatsMenu::MENU_NAME))) {
						if (baseObj->IsAmmo()) {
							actor->AddObjectToContainer(baseObj, baseObj->As<RE::ExtraDataList>(), 1, baseObj->AsReference());
							logger::debug("{} added to {}", baseObj->As<RE::TESAmmo>()->GetFullName(), actor->GetName());
						}
					}
				}
			}
		}
	}

	return RE::BSEventNotifyControl::kContinue;
}
