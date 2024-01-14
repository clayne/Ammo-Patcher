#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace AP {
    std::string logLevelStr;
    spdlog::level::level_enum logLevel = spdlog::level::trace;
    bool decision_ammo_patch;
    bool infinite_arrows;
    bool limitArrowSpeed;
    bool limitBoltSpeed;
    float arrow_speed_limiter_min;
    float arrow_speed_limiter_max;
    float bolt_speed_limiter_min;
    float bolt_speed_limiter_max;
    float arrowSpeed;
    float arrowGravity;
    float boltSpeed;
    float boltGravity;
    bool changeArrowSoundLevel;
    bool changeBoltSoundLevel;
    std::string arrowSoundLevelStr;
    std::string boltSoundLevelStr;
    RE::SOUND_LEVEL arrowSoundLevel;
    RE::SOUND_LEVEL boltSoundLevel;
    bool limitArrowDamage;
    bool limitBoltDamage;
    float arrow_damage_limiter_min;
    float arrow_damage_limiter_max;
    float bolt_damage_limiter_min;
    float bolt_damage_limiter_max;

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
                        "**********************************************************************************************"
                        "********************************");
                    log::debug(
                        "Before Patching : Name:{}|FormID:{:08X}|Damage:{} ,Projectile Name:{}|Projectile "
                        "FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}",
                        ammo->GetFullName(), ammo->GetRawFormID(), ammo->GetRuntimeData().data.damage,
                        ammo->GetRuntimeData().data.projectile->GetFullName(),
                        ammo->GetRuntimeData().data.projectile->GetRawFormID(),
                        ammo->GetRuntimeData().data.projectile->data.speed,
                        ammo->GetRuntimeData().data.projectile->data.gravity);
                    if (ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt) {  // for arrow
                        ammo->GetRuntimeData().data.projectile->data.speed = limitFloat(
                            arrowSpeed, arrow_speed_limiter_min, arrow_speed_limiter_max, limitArrowSpeed);  // speed
                        ammo->GetRuntimeData().data.projectile->data.gravity = arrowGravity;                 // gravity
                        ammo->GetRuntimeData().data.projectile->soundLevel = arrowSoundLevel;  // sound level
                        ammo->GetRuntimeData().data.damage =
                            limitFloat(ammo->GetRuntimeData().data.damage, arrow_damage_limiter_min,
                                       arrow_damage_limiter_max, limitArrowDamage);  // damage
                        ammo_patched = true;
                    }
                    if (!(ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt)) {  // for bolt
                        ammo->GetRuntimeData().data.projectile->data.speed = limitFloat(
                            boltSpeed, bolt_speed_limiter_min, bolt_speed_limiter_max, limitBoltSpeed);  // speed
                        ammo->GetRuntimeData().data.projectile->data.gravity = boltGravity;              // gravity
                        ammo->GetRuntimeData().data.projectile->soundLevel = boltSoundLevel;             // sound level
                        ammo->GetRuntimeData().data.damage =
                            limitFloat(ammo->GetRuntimeData().data.damage, bolt_damage_limiter_min,
                                       bolt_damage_limiter_max, limitBoltDamage);  // damage
                        ammo_patched = true;
                    }

                    if (ammo_patched) {
                        log::debug(
                            "After Patching : Name:{}|FormID:{:08X}|Damage:{} ,Projectile Name:{}|Projectile "
                            "FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}",
                            ammo->GetFullName(), ammo->GetRawFormID(), ammo->GetRuntimeData().data.damage,
                            ammo->GetRuntimeData().data.projectile->GetFullName(),
                            ammo->GetRuntimeData().data.projectile->GetRawFormID(),
                            ammo->GetRuntimeData().data.projectile->data.speed,
                            ammo->GetRuntimeData().data.projectile->data.gravity);
                        log::debug(
                            "******************************************************************************************"
                            "************************************");
                    }
                }
                // ammo->GetRuntimeData().data.projectile->soundLevel = RE::SOUND_LEVEL::kLoud;
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

        logLevelStr = jsonData["Logging"]["LogLevel"].get<std::string>();

        if (logLevelStr == "trace") {
            logLevel = spdlog::level::trace;
        } else if (logLevelStr == "debug") {
            logLevel = spdlog::level::debug;
        } else if (logLevelStr == "info") {
            logLevel = spdlog::level::info;
        } else if (logLevelStr == "warn") {
            logLevel = spdlog::level::warn;
        } else if (logLevelStr == "err") {
            logLevel = spdlog::level::err;
        } else if (logLevelStr == "critical") {
            logLevel = spdlog::level::critical;
        } else {
            // Default to info if the specified log level is invalid
            log::critical("Invalid log level specified in the JSON file. Defaulting to info level.");
            logLevel = spdlog::level::info;
        }

        if (decision_ammo_patch = jsonData["AMMO"]["Enable Patch"].get<bool>()) {
            if (changeArrowSoundLevel =
                    jsonData["AMMO"]["Arrow"]["Sound"]["Change Sound Level"]["Enable"].get<bool>()) {
                arrowSoundLevelStr =
                    jsonData["AMMO"]["Arrow"]["Sound"]["Change Sound Level"]["Sound Level"].get<std::string>();
            }

            if (changeBoltSoundLevel = jsonData["AMMO"]["Bolt"]["Sound"]["Change Sound Level"]["Enable"].get<bool>()) {
                boltSoundLevelStr =
                    jsonData["AMMO"]["Bolt"]["Sound"]["Change Sound Level"]["Sound Level"].get<std::string>();
            }

            if (arrowSoundLevelStr == "kLoud") {
                arrowSoundLevel = RE::SOUND_LEVEL::kLoud;
            } else if (arrowSoundLevelStr == "kNormal") {
                arrowSoundLevel = RE::SOUND_LEVEL::kNormal;
            } else if (arrowSoundLevelStr == "kSilent") {
                arrowSoundLevel = RE::SOUND_LEVEL::kSilent;
            } else if (arrowSoundLevelStr == "kVeryLoud") {
                arrowSoundLevel = RE::SOUND_LEVEL::kVeryLoud;
            } else if (arrowSoundLevelStr == "kQuiet") {
                arrowSoundLevel = RE::SOUND_LEVEL::kQuiet;
            } else {
                log::critical("Invalid Arrow Sound Level specified in the JSON file. Defaulting to kSilent.");
                arrowSoundLevel = RE::SOUND_LEVEL::kSilent;
            }

            if (boltSoundLevelStr == "kLoud") {
                boltSoundLevel = RE::SOUND_LEVEL::kLoud;
            } else if (boltSoundLevelStr == "kNormal") {
                boltSoundLevel = RE::SOUND_LEVEL::kNormal;
            } else if (boltSoundLevelStr == "kSilent") {
                boltSoundLevel = RE::SOUND_LEVEL::kSilent;
            } else if (boltSoundLevelStr == "kVeryLoud") {
                boltSoundLevel = RE::SOUND_LEVEL::kVeryLoud;
            } else if (boltSoundLevelStr == "kQuiet") {
                boltSoundLevel = RE::SOUND_LEVEL::kQuiet;
            } else {
                log::critical("Invalid Bolt Sound Level specified in the JSON file. Defaulting to kSilent.");
                boltSoundLevel = RE::SOUND_LEVEL::kSilent;
            }

            if (infinite_arrows = jsonData["AMMO"]["Arrow"]["Infinite Arrow"].get<bool>()) {
                log::info("Infinite Arrows Activated");
            }
            arrowGravity = jsonData["AMMO"]["Arrow"]["Gravity"].get<float>();
            arrowSpeed = jsonData["AMMO"]["Arrow"]["Speed"].get<float>();
            boltGravity = jsonData["AMMO"]["Bolt"]["Gravity"].get<float>();
            boltSpeed = jsonData["AMMO"]["Bolt"]["Speed"].get<float>();
            if (limitArrowSpeed = jsonData["AMMO"]["Arrow"]["Limit Speed"]["Enable"].get<bool>()) {
                arrow_speed_limiter_min = jsonData["AMMO"]["Arrow"]["Limit Speed"]["Min"].get<float>();
                arrow_speed_limiter_max = jsonData["AMMO"]["Arrow"]["Limit Speed"]["Max"].get<float>();
            }
            if (limitBoltSpeed = jsonData["AMMO"]["Bolt"]["Limit Speed"]["Enable"].get<bool>()) {
                bolt_speed_limiter_min = jsonData["AMMO"]["Bolt"]["Limit Speed"]["Mix"].get<float>();
                bolt_speed_limiter_max = jsonData["AMMO"]["Bolt"]["Limit Speed"]["Max"].get<float>();
            }
            if (limitArrowDamage = jsonData["AMMO"]["Arrow"]["Limit Damage"]["Enable"].get<bool>()) {
                arrow_damage_limiter_min = jsonData["AMMO"]["Arrow"]["Limit Damage"]["Min"].get<float>();
                arrow_damage_limiter_max = jsonData["AMMO"]["Arrow"]["Limit Damage"]["Max"].get<float>();
            }
            if (limitBoltDamage = jsonData["AMMO"]["Bolt"]["Limit Damage"]["Enable"].get<bool>()) {
                bolt_damage_limiter_min = jsonData["AMMO"]["Bolt"]["Limit Damage"]["Mix"].get<float>();
                bolt_damage_limiter_max = jsonData["AMMO"]["Bolt"]["Limit Damage"]["Max"].get<float>();
            }
        }

        log::debug("*************************************************");
        log::debug("Patch Ammo : {}", decision_ammo_patch);
        log::debug("*************************************************");
        log::debug("Infinite Arrow's : {}", infinite_arrows);
        log::debug("*************************************************");
        log::debug("Final Arrow Gravity : {}", arrowGravity);
        log::debug("Final Bolt Gravity : {}", boltGravity);
        log::debug("*************************************************");
        log::debug("Set Arrow Speed Limit : {}", limitArrowSpeed);
        log::debug("Arrow Minimum Speed Limit : {}", arrow_speed_limiter_min);
        log::debug("Arrow Maximum Speed Limit : {}", arrow_speed_limiter_max);
        log::debug("Final Arrow Speed : {}", arrowSpeed);
        log::debug("*************************************************");
        log::debug("Set Bolt Speed Limit : {}", limitBoltSpeed);
        log::debug("Bolt Minimum Speed Limit : {}", bolt_speed_limiter_min);
        log::debug("Bolt Maximum Speed Limit : {}", bolt_speed_limiter_max);
        log::debug("Final Bolt Speed : {}", boltSpeed);
        log::debug("*************************************************");
        log::debug("Change Arrow Sound Level : {}", changeArrowSoundLevel);
        log::debug("Arrow Sound Level : {}", arrowSoundLevelStr);
        log::debug("*************************************************");
        log::debug("Change Bolt Sound Level : {}", changeBoltSoundLevel);
        log::debug("Final Bolt Sound Level : {}", boltSoundLevelStr);
        log::debug("*************************************************");
        log::debug("Set Arrow Damage Limit : {}", limitArrowDamage);
        log::debug("Arrow Minimum Damage Limit : {}", arrow_damage_limiter_min);
        log::debug("Arrow Maximum Damage Limit : {}", arrow_damage_limiter_max);
        log::debug("*************************************************");
        log::debug("Set Bolt Damage Limit : {}", limitBoltDamage);
        log::debug("Bolt Minimum Damage Limit : {}", bolt_damage_limiter_min);
        log::debug("Bolt Maximum Damage Limit : {}", bolt_damage_limiter_max);
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
