#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
// using namespace SKSE;
// using namespace SKSE::log;
// using namespace SKSE::stl;
namespace fs = std::filesystem;

namespace AP {
    bool arrowPatch = true;
    bool boltPatch = true;
    bool arrowSpeedEnable = true;
    bool boltSpeedEnable = true;
    bool arrowGravityEnable = true;
    bool boltGravityEnable = true;
    bool infinite_arrows = false;
    bool limitArrowSpeed = false;
    bool limitBoltSpeed = false;
    bool limitArrowDamage = false;
    bool limitBoltDamage = false;
    float arrowDamageLimiterMin = 10.0;
    float arrowDamageLimiterMax = 1000.0;
    float boltDamageLimiterMin = 10.0;
    float boltDamageLimiterMax = 1000.0;
    float arrowSpeedLimiterMin = 3000.0;
    float arrowSpeedLimiterMax = 12000.0;
    float boltSpeedLimiterMin = 4000;
    float boltSpeedLimiterMax = 12000;
    float arrowSpeed = 9000.0;
    float arrowGravity = 0.0;
    float boltSpeed = 10800.0;
    float boltGravity = 0.0;
    bool changeArrowSoundLevel =  false;
    bool changeBoltSoundLevel = false;
    std::string arrowSoundLevelStr("kSilent");
    std::string boltSoundLevelStr("kSilent");
    RE::SOUND_LEVEL arrowSoundLevel;
    RE::SOUND_LEVEL boltSoundLevel;
    std::string folder_path ("Data/SKSE/Plugins/Ammo Patcher/");
    json jsonData;   // used for main json file
    json mergeData;  // used to merge exclusion json files
    bool hasFilesToMerge = false;
    json formIDArray;
    json tesFileArray;

    template <typename StringType>
    std::wstring toWideString(const StringType& str) {
        if constexpr (std::is_same_v<StringType, std::string_view>) {
            // Calculate the length of the wide string
            size_t wideLength = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.length()), nullptr, 0);

            // Allocate a buffer for the wide string
            std::wstring wideStr(wideLength, L'\0');

            // Convert the string
            MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.length()), &wideStr[0], static_cast<int>(wideLength));

            return wideStr;
        } else if constexpr (std::is_same_v<StringType, std::string>) {
            // Calculate the length of the wide string
            size_t wideLength = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);

            // Allocate a buffer for the wide string
            std::wstring wideStr(wideLength, L'\0');

            // Convert the string
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &wideStr[0], static_cast<int>(wideLength));

            return wideStr;
        } else {
            static_assert(std::is_same_v<StringType, std::string> || std::is_same_v<StringType, std::string_view>, "Unsupported string type");
        }
    }

    void customMessageBox(const std::string& errorString) {
        // Create the confirmation message
        std::wstring confirmationMessage = toWideString<std::string>(errorString);
        std::wstring moduleName = toWideString<std::string_view>(SKSE::PluginDeclaration::GetSingleton()->GetName());
        // Call MessageBoxW with the confirmation message
        switch (MessageBoxW(nullptr, confirmationMessage.c_str(), moduleName.c_str(), MB_YESNO | MB_ICONQUESTION)) {
            case IDNO:
                SKSE::WinAPI::TerminateProcess(SKSE::WinAPI::GetCurrentProcess(), EXIT_FAILURE);
            default:
                break;
        }
    }

    void LoadJSON() {
        std::ifstream jsonfile(std::format("Data/SKSE/Plugins/{}.json", SKSE::PluginDeclaration::GetSingleton()->GetName()));
        try {
            jsonData = json::parse(jsonfile);  // used for parsing main json file
        } catch (const json::parse_error& e) {
            /*jsonData =
                "{\"Logging\": {\"LogLevel\": \"info\"}, \"AMMO\": {\"Arrow\": {\"Enable Arrow Patch\": true, \"Change Gravity\": {\"Enable\": true, \"Gravity\": 0.0}, \"Change Speed\": {\"Enable\": true, \"Speed\": 9000.0}, \"Limit Speed\": "
                "{\"Enable\": false, \"Min\": 3000.0, \"Max\": 12000.0}, \"Limit Damage\": {\"Enable\": false, \"Min\": 10.0, \"Max\": 1000.0}, \"Sound\": {\"Change Sound Level\": {\"Enable\": false, \"Sound Level\": \"kSilent\"}}, \"Infinite "
                "Arrow\": false}, \"Bolt\": {\"Enable Bolt Patch\": true, \"Change Gravity\": {\"Enable\": true, \"Gravity\": 0.0}, \"Change Speed\": {\"Enable\": true, \"Speed\": 10800.0}, \"Limit Speed\": {\"Enable\": false, \"Min\": 4000.0, "
                "\"Max\": 12000.0}, \"Limit Damage\": {\"Enable\": false, \"Min\": 10.0, \"Max\": 1000.0}, \"Sound\": {\"Change Sound Level\": {\"Enable\": false, \"Sound Level\": \"kSilent\"}}}}}"_json;*/
            SKSE::log::error("{}", e.what());
            customMessageBox(std::format("{}. Are you sure you want to continue? if you continue, The error will be logged and a default json will be used instead.", e.what()));
        }
        try {
            if (!jsonData.empty() && !jsonData.is_null()) {
                std::string logLevelStr(jsonData["Logging"]["LogLevel"].get<std::string>());
                spdlog::level::level_enum logLevel = spdlog::level::info;

                if (logLevelStr == "trace")
                    logLevel = spdlog::level::trace;
                else if (logLevelStr == "debug")
                    logLevel = spdlog::level::debug;
                else if (logLevelStr == "info")
                    logLevel = spdlog::level::info;
                else if (logLevelStr == "warn")
                    logLevel = spdlog::level::warn;
                else if (logLevelStr == "err")
                    logLevel = spdlog::level::err;
                else if (logLevelStr == "critical")
                    logLevel = spdlog::level::critical;

                spdlog::set_level(logLevel);
                spdlog::flush_on(logLevel);

                const std::string data[20] = {"AMMO", "Arrow", "Bolt",    "Sound",   "Change Sound Level", "Enable", "Sound Level", "Change Speed", "Change Gravity", "Gravity", "Speed", "Limit Speed", "Limit Damage", "Min",
                                              "Max",          "kLoud",          "kNormal", "kSilent", "kVeryLoud",          "kQuiet"};

                arrowPatch = jsonData[data[0]][data[1]]["Enable Arrow Patch"].get<bool>();
                boltPatch = jsonData[data[0]][data[2]]["Enable Bolt Patch"].get<bool>();

                changeArrowSoundLevel = jsonData[data[0]][data[1]][data[3]][data[4]][data[5]].get<bool>();
                arrowSoundLevelStr = jsonData[data[0]][data[1]][data[3]][data[4]][data[6]].get<std::string>();

                changeBoltSoundLevel = jsonData[data[0]][data[2]][data[3]][data[4]][data[5]].get<bool>();
                boltSoundLevelStr = jsonData[data[0]][data[2]][data[3]][data[4]][data[6]].get<std::string>();

                arrowSpeedEnable = jsonData[data[0]][data[1]][data[7]][data[5]].get<bool>();
                boltSpeedEnable = jsonData[data[0]][data[2]][data[7]][data[5]].get<bool>();

                arrowGravityEnable = jsonData[data[0]][data[1]][data[8]][data[5]].get<bool>();
                boltGravityEnable = jsonData[data[0]][data[2]][data[8]][data[5]].get<bool>();

                arrowGravity = jsonData[data[0]][data[1]][data[8]][data[9]].get<float>();
                arrowSpeed = jsonData[data[0]][data[1]][data[7]][data[10]].get<float>();

                boltGravity = jsonData[data[0]][data[2]][data[8]][data[9]].get<float>();
                boltSpeed = jsonData[data[0]][data[2]][data[7]][data[10]].get<float>();

                limitArrowSpeed = jsonData[data[0]][data[1]][data[11]][data[5]].get<bool>();
                limitBoltSpeed = jsonData[data[0]][data[2]][data[11]][data[5]].get<bool>();

                limitArrowDamage = jsonData[data[0]][data[1]][data[12]][data[5]].get<bool>();
                limitBoltDamage = jsonData[data[0]][data[2]][data[12]][data[5]].get<bool>();

                arrowSpeedLimiterMin = jsonData[data[0]][data[1]][data[11]][data[13]].get<float>();
                arrowSpeedLimiterMax = jsonData[data[0]][data[1]][data[11]][data[14]].get<float>();

                boltSpeedLimiterMin = jsonData[data[0]][data[2]][data[11]][data[13]].get<float>();
                boltSpeedLimiterMax = jsonData[data[0]][data[2]][data[11]][data[14]].get<float>();

                arrowDamageLimiterMin = jsonData[data[0]][data[1]][data[12]][data[13]].get<float>();
                arrowDamageLimiterMax = jsonData[data[0]][data[1]][data[12]][data[14]].get<float>();

                boltDamageLimiterMin = jsonData[data[0]][data[2]][data[12]][data[13]].get<float>();
                boltDamageLimiterMax = jsonData[data[0]][data[2]][data[12]][data[14]].get<float>();

                infinite_arrows = jsonData[data[0]][data[1]]["Infinite Arrow"].get<bool>();

                if (arrowSoundLevelStr == data[15]) {
                    arrowSoundLevel = RE::SOUND_LEVEL::kLoud;
                } else if (arrowSoundLevelStr == data[16]) {
                    arrowSoundLevel = RE::SOUND_LEVEL::kNormal;
                } else if (arrowSoundLevelStr == data[17]) {
                    arrowSoundLevel = RE::SOUND_LEVEL::kSilent;
                } else if (arrowSoundLevelStr == data[18]) {
                    arrowSoundLevel = RE::SOUND_LEVEL::kVeryLoud;
                } else if (arrowSoundLevelStr == data[19]) {
                    arrowSoundLevel = RE::SOUND_LEVEL::kQuiet;
                } else {
                    SKSE::log::error("Invalid Arrow Sound Level specified in the JSON file. Not Patching Arrow Sound Level.");
                    changeArrowSoundLevel = false;
                }

                if (boltSoundLevelStr == data[15]) {
                    boltSoundLevel = RE::SOUND_LEVEL::kLoud;
                } else if (boltSoundLevelStr == data[16]) {
                    boltSoundLevel = RE::SOUND_LEVEL::kNormal;
                } else if (boltSoundLevelStr == data[17]) {
                    boltSoundLevel = RE::SOUND_LEVEL::kSilent;
                } else if (boltSoundLevelStr == data[18]) {
                    boltSoundLevel = RE::SOUND_LEVEL::kVeryLoud;
                } else if (boltSoundLevelStr == data[19]) {
                    boltSoundLevel = RE::SOUND_LEVEL::kQuiet;
                } else {
                    SKSE::log::error("Invalid Bolt Sound Level specified in the JSON file. Not Patching Bolt Sound Level.");
                    changeBoltSoundLevel = false;
                }

                if (infinite_arrows) {
                    SKSE::log::info("Infinite Arrows Activated");
                }
            }
            if (fs::exists(folder_path) && !fs::is_empty(folder_path)) {
                hasFilesToMerge = true;
                for (const auto& entry : fs::directory_iterator(folder_path)) {
                    if (entry.path().extension() == ".json") {
                        std::ifstream jFile(entry.path());
                        try {
                            mergeData = json::parse(jFile);
                        } catch (const json::parse_error& e) {
                            SKSE::log::error("{} parsing error : {}", entry.path().generic_string(), e.what());
                            SKSE::log::error("If you get this error, check your {}. The line above will tell where the mistake is.", entry.path().generic_string());
                            
                            customMessageBox(std::format("{}. Are you sure you want to continue? if you continue, {} will be ignored and the error will be logged.", e.what(), entry.path().generic_string()));
                            continue;
                        }
                        SKSE::log::debug("Loaded JSON from file: {}", entry.path().generic_string());

                        const std::string data1[2] = {"AMMO FormID to Exclude", "AMMO FormID to Exclude"};

                        if (mergeData[data1[0]].is_array() && mergeData[data1[1]].is_array()) {
                            
                            // Collect all elements into formIDArray
                            formIDArray.insert(formIDArray.end(), mergeData[data1[0]].begin(), mergeData[data1[1]].end());

                            // Collect all elements into tesFileArray
                            tesFileArray.insert(tesFileArray.end(), mergeData[data1[0]].begin(), mergeData[data1[1]].end());
                        }

                        if(!mergeData.empty() && !mergeData.is_null()) mergeData.clear();
                    }
                }

                
                if (!formIDArray.empty() && !formIDArray.is_null()) {
                    // Sort and remove duplicates from formIDArray
                    std::sort(formIDArray.begin(), formIDArray.end());
                    formIDArray.erase(std::unique(formIDArray.begin(), formIDArray.end()), formIDArray.end());
                }

                if (!tesFileArray.empty() &&!tesFileArray.is_null()) {
                    // Sort and remove duplicates from tesFileArray
                    std::sort(tesFileArray.begin(), tesFileArray.end());
                    tesFileArray.erase(std::unique(tesFileArray.begin(), tesFileArray.end()), tesFileArray.end());
                }

                // debug("formIDArray : \n{}",formIDArray.dump(4));
                // debug("tesFileArray : \n{}",tesFileArray.dump(4));
            } else
                SKSE::log::info("************No Exclusion will be Done************");
        
        
        } catch (const json::exception& e) {
            SKSE::log::error("{}", e.what());
            customMessageBox(std::format("{}. Want to Continue?", e.what()));
        }
        SKSE::log::info("************Finished Processing Data*************");

        const std::string data("*************************************************");

        SKSE::log::info("{}", data);
        SKSE::log::info("Patch Arrows : {}", arrowPatch);
        SKSE::log::info("Patch Bolts : {}", boltPatch);
        SKSE::log::info("{}", data);
        SKSE::log::info("Infinite Arrow's : {}", infinite_arrows);
        SKSE::log::info("{}", data);
        SKSE::log::info("Set Arrow Gravity : {}", arrowGravityEnable);
        SKSE::log::info("Arrow Gravity : {}", arrowGravity);
        SKSE::log::info("{}", data);
        SKSE::log::info("Set Bolt Gravity : {}", boltGravityEnable);
        SKSE::log::info("Bolt Gravity : {}", boltGravity);
        SKSE::log::info("{}", data);
        SKSE::log::info("Set Arrow Speed : {}", arrowSpeedEnable);
        SKSE::log::info("Arrow Speed : {}", arrowSpeed);
        SKSE::log::info("{}", data);
        SKSE::log::info("Set Bolt Speed : {}", boltSpeedEnable);
        SKSE::log::info("Bolt Speed : {}", boltSpeed);
        SKSE::log::info("{}", data);
        SKSE::log::info("Set Arrow Speed Limit : {}", limitArrowSpeed);
        SKSE::log::info("Arrow Minimum Speed Limit : {}", arrowSpeedLimiterMin);
        SKSE::log::info("Arrow Maximum Speed Limit : {}", arrowSpeedLimiterMax);
        SKSE::log::info("{}", data);
        SKSE::log::info("Limit Bolt Speed : {}", limitBoltSpeed);
        SKSE::log::info("Bolt Minimum Speed Limit : {}", boltSpeedLimiterMin);
        SKSE::log::info("Bolt Maximum Speed Limit : {}", boltSpeedLimiterMax);
        SKSE::log::info("{}", data);
        SKSE::log::info("Change Arrow Sound Level : {}", changeArrowSoundLevel);
        SKSE::log::info("Arrow Sound Level : {}", arrowSoundLevelStr);
        SKSE::log::info("{}", data);
        SKSE::log::info("Change Bolt Sound Level : {}", changeBoltSoundLevel);
        SKSE::log::info("Bolt Sound Level : {}", boltSoundLevelStr);
        SKSE::log::info("{}", data);
        SKSE::log::info("Limit Arrow Damage : {}", limitArrowDamage);
        SKSE::log::info("Arrow Minimum Damage Limit : {}", arrowDamageLimiterMin);
        SKSE::log::info("Arrow Maximum Damage Limit : {}", arrowDamageLimiterMax);
        SKSE::log::info("{}", data);
        SKSE::log::info("Limit Bolt Damage : {}", limitBoltDamage);
        SKSE::log::info("Bolt Minimum Damage Limit : {}", boltDamageLimiterMin);
        SKSE::log::info("Bolt Maximum Damage Limit : {}", boltDamageLimiterMax);
        SKSE::log::info("{}", data);
    }

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
        auto path = SKSE::log::log_directory();
        if (!path) {
            SKSE::stl::report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= std::format("{}.log", SKSE::PluginDeclaration::GetSingleton()->GetName());

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>("Ammo_Patcher", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>("Ammo_Patcher", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }

        log->set_level(spdlog::level::trace);
        log->flush_on(spdlog::level::trace);
        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%t] [%s:%#] %v");
    }

    inline float limitFloat(float value, float min_value, float max_value) { return (value < min_value) ? min_value : ((value > max_value) ? max_value : value); }

    static RE::FormID GetFormFromIdentifier(const std::string& identifier) {
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
        auto startAP = std::chrono::high_resolution_clock::now();
        if (arrowPatch || boltPatch) {
            SKSE::log::info("{} {} is starting to patch", SKSE::PluginDeclaration::GetSingleton()->GetName(), SKSE::PluginDeclaration::GetSingleton()->GetVersion());
            for (const auto ammo : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESAmmo>()) {
                if (ammo && ammo != nullptr) {
                    bool shouldPatch = true; 
                    const auto ammoProjectile = ammo->GetRuntimeData().data.projectile;
                    if (ammoProjectile && ammoProjectile != nullptr) {
                        shouldPatch = true;
                        if (hasFilesToMerge) {
                            for (const auto& ammoModName : tesFileArray) {
                                const auto ammoModNameString = ammoModName.get<std::string>();
                                if (ammoModNameString.c_str() == ammo->GetFile()->GetFilename()) {
                                    shouldPatch = false;
                                    SKSE::log::debug("******************************************************************************************************************************");
                                    SKSE::log::debug("From {} :", ammoModNameString);
                                    SKSE::log::debug("Skipping Ammo : Name:{}|FormID:{:08X}|Damage:{}|Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}", ammo->GetFullName(), ammo->GetRawFormID(),
                                                     ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed, ammoProjectile->data.gravity);
                                    SKSE::log::debug("******************************************************************************************************************************");
                                    break;
                                }
                            }
                            if (shouldPatch) {
                                for (const auto& ammoFormID : formIDArray) {
                                    auto form = GetFormFromIdentifier(ammoFormID.get<std::string>());
                                    if (form && form != (RE::FormID) nullptr) {
                                        if (ammo->GetFormID() == form) {
                                            shouldPatch = false;
                                            SKSE::log::debug("******************************************************************************************************************************");
                                            SKSE::log::debug("Skipping Ammo : Name:{}|FormID:{:08X}|Damage:{}|Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
                                                             ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed, ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
                                            SKSE::log::debug("******************************************************************************************************************************");
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
                                if ((arrowPatch || boltPatch) && ammoPatched) {
                                    SKSE::log::debug("******************************************************************************************************************************");
                                    SKSE::log::debug("Before Patching : Name:{}|FormID:{:08X}|Damage:{}|Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
                                                     ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed, ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
                                }

                                if (ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt) {  // for arrow
                                    if (arrowPatch) {
                                        if (changeArrowSoundLevel) {  // set sound level
                                            ammoProjectile->soundLevel = arrowSoundLevel;
                                            SKSE::log::debug("changed Arrow Sound Level");
                                        }
                                        if (arrowSpeedEnable) {  // set speed
                                            ammoProjectile->data.speed = arrowSpeed;
                                            SKSE::log::debug("Changed Arrow Speed");
                                        }
                                        if (arrowGravityEnable) {  // set gravity
                                            ammoProjectile->data.gravity = arrowGravity;
                                            SKSE::log::debug("Changed Arrow Gravity");
                                        }
                                        if (limitArrowDamage) {  // limit damage
                                            ammo->GetRuntimeData().data.damage = limitFloat(ammo->GetRuntimeData().data.damage, arrowDamageLimiterMin, arrowDamageLimiterMax);
                                            SKSE::log::debug("Limited Arrow Damage");
                                        }
                                        if (limitArrowSpeed) {  // limit speed
                                            ammoProjectile->data.speed = limitFloat(ammoProjectile->data.speed, arrowSpeedLimiterMin, arrowSpeedLimiterMax);
                                            SKSE::log::debug("Limited Arrow Level");
                                        }
                                    }
                                }

                                if (!(ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt)) {  // for bolt
                                    if (boltPatch) {
                                        if (changeBoltSoundLevel) {  // set sound level of bolt
                                            ammoProjectile->soundLevel = boltSoundLevel;
                                            SKSE::log::debug("changed Bolt Sound Level");
                                        }
                                        if (boltSpeedEnable) {  // set speed of bolt
                                            ammoProjectile->data.speed = boltSpeed;
                                            SKSE::log::debug("Changed Bolt Speed");
                                        }
                                        if (boltGravityEnable) {  // set gravity of bolt
                                            ammoProjectile->data.gravity = boltGravity;
                                            SKSE::log::debug("Changed Bolt Speed");
                                        }
                                        if (limitBoltSpeed) {  // limit speed of bolt
                                            ammoProjectile->data.speed = limitFloat(ammoProjectile->data.speed, boltSpeedLimiterMin, boltSpeedLimiterMax);
                                            SKSE::log::debug("Limited Bolt Speed");
                                        }
                                        if (limitBoltDamage) {  // limit damage of bolt
                                            ammo->GetRuntimeData().data.damage = limitFloat(ammo->GetRuntimeData().data.damage, boltDamageLimiterMin, boltDamageLimiterMax);
                                            SKSE::log::debug("Limited Bolt Damage");
                                        }
                                    }
                                }

                                if ((arrowPatch || boltPatch) && ammoPatched) {
                                    SKSE::log::debug("After Patching : Name:{}|FormID:{:08X}|Damage:{}|Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
                                                     ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed, ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
                                    SKSE::log::debug("******************************************************************************************************************************");
                                }
                            }
                        }
                    } else
                        SKSE::log::info("PROJ Record for {} with FormID {:08X} from file {} is NULL", ammo->GetFullName(), ammo->GetRawFormID(), ammo->GetFile()->GetFilename());
                } else
                    SKSE::log::debug("This Iteration's Ammo is nullptr");
            }
            SKSE::log::info("{} {} has finished Patching", SKSE::PluginDeclaration::GetSingleton()->GetName(), SKSE::PluginDeclaration::GetSingleton()->GetVersion());
            if (!formIDArray.empty()) formIDArray.clear();
            if (!tesFileArray.empty()) tesFileArray.clear();
        } else
            SKSE::log::info("Not Patching");
        auto nanosecondsTakenForAP = std::chrono::duration(std::chrono::high_resolution_clock::now() - startAP);
        SKSE::log::info("Time Taken in ammo_patch() totally is {} nanoseconds or {} microseconds or {} milliseconds or {} seconds or {} minutes", nanosecondsTakenForAP.count(),
                        std::chrono::duration_cast<std::chrono::microseconds>(nanosecondsTakenForAP).count(), std::chrono::duration_cast<std::chrono::milliseconds>(nanosecondsTakenForAP).count(),
                        std::chrono::duration_cast<std::chrono::seconds>(nanosecondsTakenForAP).count(), std::chrono::duration_cast<std::chrono::minutes>(nanosecondsTakenForAP).count());
    }

    struct AP_Event final : public RE::BSTEventSink<RE::TESPlayerBowShotEvent> {
        RE::BSEventNotifyControl ProcessEvent(const RE::TESPlayerBowShotEvent* event, RE::BSTEventSource<RE::TESPlayerBowShotEvent>*) override {
            if (event && event != nullptr) {
                auto ammo = RE::TESForm::LookupByID<RE::TESAmmo>(event->ammo);
                RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(ammo->As<RE::TESBoundObject>(), ammo->As<RE::ExtraDataList>(), 1, ammo->AsReference());
                SKSE::log::debug("{} added to Player", ammo->GetFullName());
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void InitializeMessaging() {
        if (!SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
                if (message->type == SKSE::MessagingInterface::kDataLoaded) ammo_patch();
            })) {
            SKSE::stl::report_and_fail("Unable to register message listener.");
        }
    }
}  // namespace AP


/*
 * This if the main callback for initializing your SKSE plugin, called just before Skyrim runs its main function.
 *
 * <p>
 * This is your main entry point to your plugin, where you should initialize everything you need. Many things can't be
 * done yet here, since Skyrim has not initialized and the Windows loader lock is not released (so don't do any
 * multithreading). But you can register to listen for messages for later stages of Skyrim startup to perform such
 * tasks.
 * </p>
 */
SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    auto startSPL = std::chrono::high_resolution_clock::now();
    AP::InitializeLogging();

    AP::LoadJSON();
    SKSE::Init(a_skse);

    AP::InitializeMessaging();
    if (AP::infinite_arrows) {
        const auto eventSink = new AP::AP_Event();
        auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        eventSourceHolder->AddEventSink<RE::TESPlayerBowShotEvent>(eventSink);
    }
    auto nanosecondsTakenForSPL = std::chrono::duration(std::chrono::high_resolution_clock::now() - startSPL);
    
    
    SKSE::log::info("Time Taken in SKSEPluginLoad(const SKSE::LoadInterface* a_skse) totally is {} nanoseconds or {} microseconds or {} milliseconds or {} seconds or {} minutes", nanosecondsTakenForSPL.count(),
                    std::chrono::duration_cast<std::chrono::microseconds>(nanosecondsTakenForSPL).count(), std::chrono::duration_cast<std::chrono::milliseconds>(nanosecondsTakenForSPL).count(),
                    std::chrono::duration_cast<std::chrono::seconds>(nanosecondsTakenForSPL).count(), std::chrono::duration_cast<std::chrono::minutes>(nanosecondsTakenForSPL).count());
    return true;
}
