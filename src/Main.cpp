#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;
namespace fs = std::filesystem;

namespace AP {
    std::string logLevelStr = "info";
    spdlog::level::level_enum logLevel;
    bool arrowPatch;
    bool boltPatch;
    bool infinite_arrows;
    bool limitArrowSpeed;
    bool limitBoltSpeed;
    float arrowSpeedLimiterMin;
    float arrowSpeedLimiterMax;
    float boltSpeedLimiterMin;
    float boltSpeedLimiterMax;
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
    float arrowDamageLimiterMin;
    float arrowDamageLimiterMax;
    float boltDamageLimiterMin;
    float boltDamageLimiterMax;
    std::string folder_path = "Data/SKSE/Plugins/Ammo Patcher/";
    json jsonData;    // used for main json file
    json mergeData;  // used to merge exclusion json files
    bool hasFilesToMerge = false;
    json formIDArray = json::array();
    json tesFileArray = json::array();
    std::string jsonDataString = "";
    bool arrowSpeedEnable;
    bool boltSpeedEnable;
    bool arrowGravityEnable;
    bool boltGravityEnable;

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
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }

        std::ifstream jsonfile(std::format("Data/SKSE/Plugins/{}.json", PluginDeclaration::GetSingleton()->GetName()));

        try {
            jsonData = json::parse(jsonfile);  // used for parsing main json file
        } catch (const json::parse_error e) {
            jsonDataString =
                "{\"Logging\": {\"LogLevel\": \"info\"}, \"AMMO\": {\"Arrow\": {\"Enable Arrow Patch\": true, \"Change Gravity\": {\"Enable\": true, \"Gravity\": 0.0}, \"Change Speed\": {\"Enable\": true, \"Speed\": 9000.0}, \"Limit Speed\": "
                "{\"Enable\": false, \"Min\": 3000.0, \"Max\": 12000.0}, \"Limit Damage\": {\"Enable\": false, \"Min\": 10.0, \"Max\": 1000.0}, \"Sound\": {\"Change Sound Level\": {\"Enable\": false, \"Sound Level\": \"kSilent\"}}, \"Infinite "
                "Arrow\": false}, \"Bolt\": {\"Enable Bolt Patch\": true, \"Change Gravity\": {\"Enable\": true, \"Gravity\": 0.0}, \"Change Speed\": {\"Enable\": true, \"Speed\": 10800.0}, \"Limit Speed\": {\"Enable\": false, \"Min\": 4000.0, "
                "\"Max\": 12000.0}, \"Limit Damage\": {\"Enable\": false, \"Min\": 10.0, \"Max\": 1000.0}, \"Sound\": {\"Change Sound Level\": {\"Enable\": false, \"Sound Level\": \"kSilent\"}}}}}";


            jsonData = json::parse(jsonDataString);
            jsonDataString = e.what();
        }
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
            logLevel = spdlog::level::info;
        }

        log->set_level(logLevel);
        log->flush_on(logLevel);
        set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    inline float limitFloat(float value, float min_value, float max_value) { return (value < min_value) ? min_value : ((value > max_value) ? max_value : value); }

    RE::FormID GetFormFromIdentifier(const std::string& identifier) {
        RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
        auto delimiter = identifier.find('|');
        if (delimiter != std::string::npos) {
            std::string modName = identifier.substr(0, delimiter);
            std::string modForm = identifier.substr(delimiter + 1);
            uint32_t formID = std::stoul(modForm, nullptr, 16) & 0xFFFFFF;
            const RE::TESFile* mod = (RE::TESFile*)dataHandler->LookupModByName(modName.c_str());
            if (mod && mod != nullptr) {
                if (mod->IsLight()) formID = std::stoul(modForm, nullptr, 16) & 0xFFF;
                return dataHandler->LookupForm(formID, modName.c_str())->GetFormID();
            }
        }
        return (RE::FormID) nullptr;
    }

    void ammo_patch() {
        if (arrowPatch || boltPatch) {
            bool shouldPatch = true;
            info("{} {} is starting to patch", PluginDeclaration::GetSingleton()->GetName(), PluginDeclaration::GetSingleton()->GetVersion());
            for (const auto ammo : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESAmmo>()) {
                const auto ammoProjectile = ammo->GetRuntimeData().data.projectile;
                shouldPatch = true;
                if (hasFilesToMerge) {
                    for (const auto& ammoModName : tesFileArray) {
                        const auto ammoModNameString = ammoModName.get<std::string>();
                        if (ammoModNameString.c_str() == ammo->GetFile()->GetFilename()) {
                            shouldPatch = false;
                            debug("******************************************************************************************************************************");
                            debug("From {} :", ammoModNameString);
                            debug("Skipping Ammo : Name:{}|FormID:{:08X}|Damage:{} ,Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}", ammo->GetFullName(), ammo->GetRawFormID(), ammo->GetRuntimeData().data.damage,
                                  ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed,
                                  ammoProjectile->data.gravity);
                            debug("******************************************************************************************************************************");
                            break;
                        }
                    }
                    if (shouldPatch) {
                        for (const auto& ammoFormID : formIDArray) {
                            auto form = GetFormFromIdentifier(ammoFormID.get<std::string>());
                            if (form) {
                                if (ammo->GetFormID() == form) {
                                    shouldPatch = false;
                                    debug("******************************************************************************************************************************");
                                    debug("Skipping Ammo : Name:{}|FormID:{:08X}|Damage:{} ,Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
                                          ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed,
                                          ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
                                    debug("******************************************************************************************************************************");
                                    break;
                                }
                            }
                        }
                    }
                }
                if (shouldPatch) {
                    if (!(ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonPlayable)) {
                        bool ammoPatched = false;
						if (changeArrowSoundLevel || arrowSpeedEnable || arrowGravityEnable || limitArrowDamage || limitArrowSpeed) ammoPatched = true;
                        if (changeBoltSoundLevel || boltSpeedEnable || boltGravityEnable || limitBoltDamage || limitBoltSpeed) ammoPatched = true;
                        if((arrowPatch || boltPatch) && ammoPatched){
							debug("******************************************************************************************************************************");
							debug("Before Patching : Name:{}|FormID:{:08X}|Damage:{} ,Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
                              ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed,
							  ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
						}

                        if (ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt) {  // for arrow
                            if (arrowPatch) {
                                if (changeArrowSoundLevel) {  // set sound level
                                    ammoProjectile->soundLevel = arrowSoundLevel;
                                    debug("changed Arrow Sound Level");
                                }
                                if (arrowSpeedEnable) {  // set speed
                                    ammoProjectile->data.speed = arrowSpeed;
                                    debug("Changed Arrow Speed");
                                }
                                if (arrowGravityEnable) {  // set gravity
                                    ammoProjectile->data.gravity = arrowGravity;
                                    debug("Changed Arrow Gravity");
                                }
                                if (limitArrowDamage) {  // limit damage
                                    ammo->GetRuntimeData().data.damage = limitFloat(ammo->GetRuntimeData().data.damage, arrowDamageLimiterMin, arrowDamageLimiterMax);
                                    debug("Limited Arrow Damage");
                                }
                                if (limitArrowSpeed) {  // limit speed
                                    ammoProjectile->data.speed = limitFloat(ammoProjectile->data.speed, arrowSpeedLimiterMin, arrowSpeedLimiterMax);
                                    debug("Limited Arrow Level");
                                }
                            }
                        }

                        if (!(ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt)) {  // for bolt
                            if (boltPatch) {
                                if (changeBoltSoundLevel) {  // set sound level of bolt
                                    ammoProjectile->soundLevel = boltSoundLevel;
                                    debug("changed Bolt Sound Level");
                                }
                                if (boltSpeedEnable) {  // set speed of bolt
                                    ammoProjectile->data.speed = boltSpeed;
                                    debug("Changed Bolt Speed");
                                }
                                if (boltGravityEnable) {  // set gravity of bolt
                                    ammoProjectile->data.gravity = boltGravity;
                                    debug("Changed Bolt Speed");
                                }
                                if (limitBoltSpeed) {  // limit speed of bolt
                                    ammoProjectile->data.speed = limitFloat(ammoProjectile->data.speed, boltSpeedLimiterMin, boltSpeedLimiterMax);
                                    debug("Limited Bolt Speed");
                                }
                                if (limitBoltDamage) {  // limit damage of bolt
                                    ammo->GetRuntimeData().data.damage = limitFloat(ammo->GetRuntimeData().data.damage, boltDamageLimiterMin, boltDamageLimiterMax);
                                    debug("Limited Bolt Damage");
                                }
                            }
                        }

                        if((arrowPatch || boltPatch) && ammoPatched){
                            debug("After Patching : Name:{}|FormID:{:08X}|Damage:{} ,Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
                                  ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed,
                                  ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
                            debug("******************************************************************************************************************************");
                        }
                    }
                }
            }
        }
        info("{} {} has finished Patching", PluginDeclaration::GetSingleton()->GetName(), PluginDeclaration::GetSingleton()->GetVersion());
        if(!formIDArray.empty()) formIDArray.clear();
        if (!tesFileArray.empty()) tesFileArray.clear();
    }

    struct AP_Event final : public RE::BSTEventSink<RE::TESPlayerBowShotEvent> {
        RE::BSEventNotifyControl ProcessEvent(const RE::TESPlayerBowShotEvent* event, RE::BSTEventSource<RE::TESPlayerBowShotEvent>*) override {
            if (event && infinite_arrows) {
                auto ammo = RE::TESForm::LookupByID<RE::TESAmmo>(event->ammo);
                RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(ammo->As<RE::TESBoundObject>(), ammo->As<RE::ExtraDataList>(), 1, ammo->AsReference());
                debug("{} added to Player", ammo->GetFullName());
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
        if (jsonDataString != "") {
            error("{}", jsonDataString);
            error("Loading Default {}.json", PluginDeclaration::GetSingleton()->GetName());
        }

        info("finished loading {}.json", PluginDeclaration::GetSingleton()->GetName());

        info("Log Level is {}", logLevelStr);

        arrowPatch = jsonData["AMMO"]["Arrow"]["Enable Arrow Patch"].get<bool>();
        boltPatch = jsonData["AMMO"]["Bolt"]["Enable Bolt Patch"].get<bool>();

        changeArrowSoundLevel = jsonData["AMMO"]["Arrow"]["Sound"]["Change Sound Level"]["Enable"].get<bool>();
        arrowSoundLevelStr = jsonData["AMMO"]["Arrow"]["Sound"]["Change Sound Level"]["Sound Level"].get<std::string>();

        changeBoltSoundLevel = jsonData["AMMO"]["Bolt"]["Sound"]["Change Sound Level"]["Enable"].get<bool>();
        boltSoundLevelStr = jsonData["AMMO"]["Bolt"]["Sound"]["Change Sound Level"]["Sound Level"].get<std::string>();

        infinite_arrows = jsonData["AMMO"]["Arrow"]["Infinite Arrow"].get<bool>();

        arrowSpeedEnable = jsonData["AMMO"]["Arrow"]["Change Speed"]["Enable"].get<bool>();
        boltSpeedEnable = jsonData["AMMO"]["Bolt"]["Change Speed"]["Enable"].get<bool>();

        arrowGravityEnable = jsonData["AMMO"]["Arrow"]["Change Gravity"]["Enable"].get<bool>();
        boltGravityEnable = jsonData["AMMO"]["Bolt"]["Change Gravity"]["Enable"].get<bool>();

        arrowGravity = jsonData["AMMO"]["Arrow"]["Change Gravity"]["Gravity"].get<float>();
        arrowSpeed = jsonData["AMMO"]["Arrow"]["Change Speed"]["Speed"].get<float>();

        boltGravity = jsonData["AMMO"]["Bolt"]["Change Gravity"]["Gravity"].get<float>();
        boltSpeed = jsonData["AMMO"]["Bolt"]["Change Speed"]["Speed"].get<float>();

        limitArrowSpeed = jsonData["AMMO"]["Arrow"]["Limit Speed"]["Enable"].get<bool>();
        limitBoltSpeed = jsonData["AMMO"]["Bolt"]["Limit Speed"]["Enable"].get<bool>();

        limitArrowDamage = jsonData["AMMO"]["Arrow"]["Limit Damage"]["Enable"].get<bool>();
        limitBoltDamage = jsonData["AMMO"]["Bolt"]["Limit Damage"]["Enable"].get<bool>();

        arrowSpeedLimiterMin = jsonData["AMMO"]["Arrow"]["Limit Speed"]["Min"].get<float>();
        arrowSpeedLimiterMax = jsonData["AMMO"]["Arrow"]["Limit Speed"]["Max"].get<float>();

        boltSpeedLimiterMin = jsonData["AMMO"]["Bolt"]["Limit Speed"]["Min"].get<float>();
        boltSpeedLimiterMax = jsonData["AMMO"]["Bolt"]["Limit Speed"]["Max"].get<float>();

        arrowDamageLimiterMin = jsonData["AMMO"]["Arrow"]["Limit Damage"]["Min"].get<float>();
        arrowDamageLimiterMax = jsonData["AMMO"]["Arrow"]["Limit Damage"]["Max"].get<float>();

        boltDamageLimiterMin = jsonData["AMMO"]["Bolt"]["Limit Damage"]["Min"].get<float>();
        boltDamageLimiterMax = jsonData["AMMO"]["Bolt"]["Limit Damage"]["Max"].get<float>();

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
            error("Invalid Arrow Sound Level specified in the JSON file. Not Patching Arrow's Sound Level.");
            changeArrowSoundLevel = false;
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
            error("Invalid Bolt Sound Level specified in the JSON file. Not Patching Bolt Sound Level.");
            changeBoltSoundLevel = false;
        }

        if (infinite_arrows) {
            info("Infinite Arrows Activated");
        }

        if (fs::exists(folder_path) && !fs::is_empty(folder_path)) {
            hasFilesToMerge = true;
            for (const auto& entry : fs::directory_iterator(folder_path)) {
                if (entry.path().extension() == ".json") {
                    std::ifstream jFile(entry.path());
                    try {
                        mergeData = json::parse(jFile);
                    } catch (const json::parse_error e) {
                        error("{} parsing error : {}", entry.path().generic_string(), e.what());
                        error("If you get this error, check your {}. The line above will tell where the mistake is.", entry.path().generic_string());
                    }
                    debug("Loaded JSON from file: {}", entry.path().generic_string());

                    if (mergeData["AMMO FormID to Exclude"].is_array() && mergeData["Mod File(s) to Exclude"].is_array()) {
                        // Collect all elements into formIDArray
                        formIDArray.insert(formIDArray.end(), mergeData["AMMO FormID to Exclude"].begin(), mergeData["AMMO FormID to Exclude"].end());

                        // Collect all elements into tesFileArray
                        tesFileArray.insert(tesFileArray.end(), mergeData["Mod File(s) to Exclude"].begin(), mergeData["Mod File(s) to Exclude"].end());
                    }

                    mergeData.clear();
                }
            }
        }

        // Sort and remove duplicates from formIDArray
        std::sort(formIDArray.begin(), formIDArray.end());
        formIDArray.erase(std::unique(formIDArray.begin(), formIDArray.end()), formIDArray.end());

        // Sort and remove duplicates from tesFileArray
        std::sort(tesFileArray.begin(), tesFileArray.end());
        tesFileArray.erase(std::unique(tesFileArray.begin(), tesFileArray.end()), tesFileArray.end());

        // debug("formIDArray : {}",formIDArray.dump(4));
        // debug("no of items in formIDArray : {}",formIDArray.size());
        // debug("tesFileArray : {}",tesFileArray.dump(4));
        // debug("no of items in tesFileArray : {}",tesFileArray.size());

        if (!(hasFilesToMerge)) info("************No Exclusion will be Done************");

        info("************Finished Processing Data*************");

        info("*************************************************");
        info("Patch Arrows : {}", arrowPatch);
        info("Patch Bolts : {}", boltPatch);
        info("*************************************************");
        info("Infinite Arrow's : {}", infinite_arrows);
        info("*************************************************");
        info("Set Arrow Gravity : {}", arrowGravityEnable);
        info("Arrow Gravity : {}", arrowGravity);
        info("*************************************************");
        info("Set Bolt Gravity : {}", boltGravityEnable);
        info("Bolt Gravity : {}", boltGravity);
        info("*************************************************");
        info("Set Arrow Speed : {}", arrowSpeedEnable);
        info("Arrow Speed : {}", arrowSpeed);
        info("*************************************************");
        info("Set Bolt Speed : {}", boltSpeedEnable);
        info("Bolt Speed : {}", boltSpeed);
        info("*************************************************");
        info("Set Arrow Speed Limit : {}", limitArrowSpeed);
        info("Arrow Minimum Speed Limit : {}", arrowSpeedLimiterMin);
        info("Arrow Maximum Speed Limit : {}", arrowSpeedLimiterMax);
        info("*************************************************");
        info("Limit Bolt Speed : {}", limitBoltSpeed);
        info("Bolt Minimum Speed Limit : {}", boltSpeedLimiterMin);
        info("Bolt Maximum Speed Limit : {}", boltSpeedLimiterMax);
        info("*************************************************");
        info("Change Arrow Sound Level : {}", changeArrowSoundLevel);
        info("Arrow Sound Level : {}", arrowSoundLevelStr);
        info("*************************************************");
        info("Change Bolt Sound Level : {}", changeBoltSoundLevel);
        info("Bolt Sound Level : {}", boltSoundLevelStr);
        info("*************************************************");
        info("Limit Arrow Damage : {}", limitArrowDamage);
        info("Arrow Minimum Damage Limit : {}", arrowDamageLimiterMin);
        info("Arrow Maximum Damage Limit : {}", arrowDamageLimiterMax);
        info("*************************************************");
        info("Limit Bolt Damage : {}", limitBoltDamage);
        info("Bolt Minimum Damage Limit : {}", boltDamageLimiterMin);
        info("Bolt Maximum Damage Limit : {}", boltDamageLimiterMax);
        info("*************************************************");
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
    info("{} {} is loading...", PluginDeclaration::GetSingleton()->GetName(), PluginDeclaration::GetSingleton()->GetVersion());
    AP::LoadJSON();
    Init(skse);

    AP::InitializeMessaging();

    const auto eventSink = new AP::AP_Event();
    auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
    eventSourceHolder->AddEventSink<RE::TESPlayerBowShotEvent>(eventSink);

    return true;
}
