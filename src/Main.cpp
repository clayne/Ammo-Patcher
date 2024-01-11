#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace AP {
    int logLevelint;
    spdlog::level::level_enum logLevel = spdlog::level::trace;
    bool decision_ammo_patch;
    bool infinite_arrows;
    bool set_arrow_speed_limit;
    bool set_bolt_speed_limit;
    float arrow_limiter_min;
    float arrow_limiter_max;
    float bolt_limiter_min;
    float bolt_limiter_max;
    float arrowSpeed;
    float arrowGravity;
    float boltSpeed;
    float boltGravity;

    /**
     * Setup logging.
     *
     * <p>
     * Logging is important to track issues. CommonLibSSE bundles functionality for spdlog, a common C++ logging
     * framework. Here we initialize it, using values from the configuration file. This includes support for a debug
     * logger that shows output in your IDE when it has a debugger attached to Skyrim, as well as a file logger which
     * writes data to the standard SKSE logging directory at <code>Documents/My Games/Skyrim Special Edition/SKSE</code>
     * (or <code>Skyrim VR</code> if you are using VR).
     * </p>
     */
    void InitializeLogging() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= std::format("{}.log", PluginDeclaration::GetSingleton()->GetName());

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }

        log->set_level(logLevel);
        log->flush_on(logLevel);
        set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    float limitFloat(float value, float minValue, float maxValue, bool shouldLimit) {
        if (shouldLimit) {
            if (value < minValue) {
                return minValue;
            } else if (value > maxValue) {
                return maxValue;
            }
        }
        return value;
    }

    void ammo_patch() {
        if (decision_ammo_patch) {
            bool ammo_patched = false;
            info("{} {} is starting to patch", PluginDeclaration::GetSingleton()->GetName(),
                 PluginDeclaration::GetSingleton()->GetVersion());
            for (const auto ammo : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESAmmo>()) {
                ammo_patched = false;

                if (!(ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonPlayable)) {
                    log::debug(
                        "Before Patching : Name:{}|FormID:{:08X}|Damage:{} ,Projectile Name:{}|Projectile "
                        "FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{} ",
                        ammo->GetFullName(), ammo->GetRawFormID(), ammo->GetRuntimeData().data.damage,
                        ammo->GetRuntimeData().data.projectile->GetFullName(),
                        ammo->GetRuntimeData().data.projectile->GetRawFormID(),
                        ammo->GetRuntimeData().data.projectile->data.speed,
                        ammo->GetRuntimeData().data.projectile->data.gravity);
                    if (ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt) {  // arrow
                        ammo->GetRuntimeData().data.projectile->data.speed =
                            limitFloat(arrowSpeed, arrow_limiter_min, arrow_limiter_max, set_arrow_speed_limit);
                        ammo->GetRuntimeData().data.projectile->data.gravity = arrowGravity;
                        ammo_patched = true;
                    }
                    if (!(ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt)) {  // bolt
                        ammo->GetRuntimeData().data.projectile->data.speed =
                            limitFloat(boltSpeed, bolt_limiter_min, bolt_limiter_max, set_bolt_speed_limit);
                        ammo->GetRuntimeData().data.projectile->data.gravity = boltGravity;
                        ammo_patched = true;
                    }

                    if (ammo_patched) {
                        log::debug(
                            "After Patching : Name:{}|FormID:{:08X}|Damage:{} ,Projectile Name:{}|Projectile "
                            "FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{} ",
                            ammo->GetFullName(), ammo->GetRawFormID(), ammo->GetRuntimeData().data.damage,
                            ammo->GetRuntimeData().data.projectile->GetFullName(),
                            ammo->GetRuntimeData().data.projectile->GetRawFormID(),
                            ammo->GetRuntimeData().data.projectile->data.speed,
                            ammo->GetRuntimeData().data.projectile->data.gravity);
                    }
                }
            }
        }
        log::info("{} {} has finished Patching", PluginDeclaration::GetSingleton()->GetName(),
                  PluginDeclaration::GetSingleton()->GetVersion());
    }

    struct AP_Event final : public RE::BSTEventSink<RE::TESPlayerBowShotEvent> {
        RE::BSEventNotifyControl ProcessEvent(const RE::TESPlayerBowShotEvent* event,
                                              RE::BSTEventSource<RE::TESPlayerBowShotEvent>*) override {
            if (event && infinite_arrows) {
                auto ammo = RE::TESForm::LookupByID<RE::TESAmmo>(event->ammo);
                RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(
                    ammo->As<RE::TESBoundObject>(), ammo->As<RE::ExtraDataList>(), 1, ammo->AsReference());
                log::debug("{} added to Player", ammo->GetFullName());
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void InitializeMessaging() {
        if (!GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message) {
                if (message->type == MessagingInterface::kDataLoaded) ammo_patch();
            })) {
            stl::report_and_fail("Unable to register message listener.");
        }
    }

    void LoadJSON() {
        log::info("Starting to load {}.json", PluginDeclaration::GetSingleton()->GetName());
        std::ifstream jsonfile(R"(Data\SKSE\Plugins\Ammo_Patcher.json)");
        json jsonData = json::parse(jsonfile);

        logLevelint = jsonData["Logging"]["LogLevel"].get<int>();

        if (logLevelint == 0) {
            logLevel = spdlog::level::trace;
        } else if (logLevelint == 1) {
            logLevel = spdlog::level::debug;
        } else if (logLevelint == 2) {
            logLevel = spdlog::level::info;
        } else if (logLevelint == 3) {
            logLevel = spdlog::level::warn;
        } else if (logLevelint == 4) {
            logLevel = spdlog::level::err;
        } else if (logLevelint == 5) {
            logLevel = spdlog::level::critical;
        } else {
            // Default to info if the specified log level is invalid
            logLevel = spdlog::level::info;
        }

        if (infinite_arrows = jsonData["InfiniteArrow"].get<bool>()) {
            log::info("Infinite Arrows Activated");
        }
        if (decision_ammo_patch = jsonData["AMMO"]["enablePatch"].get<bool>()) {
            arrowGravity = jsonData["AMMO"]["Arrow"]["arrowGravity"].get<float>();
            arrowSpeed = jsonData["AMMO"]["Arrow"]["arrowSpeed"].get<float>();
            boltGravity = jsonData["AMMO"]["Bolt"]["boltGravity"].get<float>();
            boltSpeed = jsonData["AMMO"]["Bolt"]["boltSpeed"].get<float>();
            if (set_arrow_speed_limit = jsonData["LimitArrowSpeed"]["Enable"].get<bool>()) {
                arrow_limiter_min = jsonData["LimitArrowSpeed"]["arrowSpeedMin"].get<float>();
                arrow_limiter_max = jsonData["LimitArrowSpeed"]["arrowSpeedMax"].get<float>();
            }
            if (set_bolt_speed_limit = jsonData["LimitBoltSpeed"]["Enable"].get<bool>()) {
                bolt_limiter_min = jsonData["LimitBoltSpeed"]["boltSpeedMin"].get<float>();
                bolt_limiter_max = jsonData["LimitBoltSpeed"]["boltSpeedMax"].get<float>();
            }
        }

        log::debug("*************************************************");
        log::debug("Patch Ammo : {}", decision_ammo_patch);
        log::debug("*************************************************");
        log::debug("Infinite Arrow's : {}", infinite_arrows);
        log::debug("*************************************************");
        log::debug("Set Arrow Speed Limit : {}", set_arrow_speed_limit);
        log::debug("Set Bolt Speed Limit : {}", set_bolt_speed_limit);
        log::debug("*************************************************");
        log::debug("Arrow Minimum Speed : {}", arrow_limiter_min);
        log::debug("Arrow Maximum Speed : {}", arrow_limiter_max);
        log::debug("Actual Arrow Speed : {}", arrowSpeed);
        log::debug("Actual Arrow Gravity : {}", arrowGravity);
        log::debug("*************************************************");
        log::debug("Bolt Minimum Speed : {}", bolt_limiter_min);
        log::debug("Bolt Maximum Speed : {}", bolt_limiter_max);
        log::debug("Actual Bolt Speed : {}", boltSpeed);
        log::debug("Actual Bolt Gravity : {}", boltGravity);
        log::debug("*************************************************");
        log::info("finished loading {}.json", PluginDeclaration::GetSingleton()->GetName());
    }
}  // namespace AP

/**
 * This if the main callback for initializing your SKSE plugin, called just before Skyrim runs its main function.
 *
 * <p>
 * This is your main entry point to your plugin, where you should initialize everything you need. Many things can't be
 * done yet here, since Skyrim has not initialized and the Windows loader lock is not released (so don't do any
 * multithreading). But you can register to listen for messages for later stages of Skyrim startup to perform such
 * tasks.
 * </p>
 */
SKSEPluginLoad(const LoadInterface* skse) {
    AP::InitializeLogging();
    log::info("{} {} is loading...", PluginDeclaration::GetSingleton()->GetName(),
              PluginDeclaration::GetSingleton()->GetVersion());
    AP::LoadJSON();
    Init(skse);

    AP::InitializeMessaging();

    const auto eventSink = new AP::AP_Event();
    auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
    eventSourceHolder->AddEventSink<RE::TESPlayerBowShotEvent>(eventSink);

    return true;
}
