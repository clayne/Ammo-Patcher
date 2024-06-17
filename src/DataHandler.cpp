#include "DataHandler.h"
#include "utils.h"

DataHandler* DataHandler::GetSingleton()
{
	static DataHandler Singleton;
	return std::addressof(Singleton);
}

void DataHandler::LoadJson()
{
	std::ifstream jsonfile(std::format("Data/SKSE/Plugins/{}.json", SKSE::PluginDeclaration::GetSingleton()->GetName()));
	try {
		_JsonData = nJson::parse(jsonfile);  // used for parsing main json file
	} catch (const nJson::parse_error& e) {
		_JsonData =
			R"({"Logging":{"LogLevel":"info"},"AMMO":{"Infinite AMMO":{"Player":true,"Teammate":true},"Arrow":{"Enable Arrow Patch":true,"Change Gravity":{"Enable":true,"Gravity":0.0},"Change Speed":{"Enable":true,"Speed":9000.0},"Limit Speed":{"Enable":false,"Min":3000.0,"Max":12000.0},"Limit Damage":{"Enable":false,"Min":10.0,"Max":1000.0},"Sound":{"Change Sound Level":{"Enable":false,"Sound Level":"kSilent"}}},"Bolt":{"Enable Bolt Patch":true,"Change Gravity":{"Enable":true,"Gravity":0.0},"Change Speed":{"Enable":true,"Speed":10800.0},"Limit Speed":{"Enable":false,"Min":4000.0,"Max":12000.0},"Limit Damage":{"Enable":false,"Min":10.0,"Max":1000.0},"Sound":{"Change Sound Level":{"Enable":false,"Sound Level":"kSilent"}}}}})"_json;
		logger::error("{}", e.what());
		Utils::customMessageBox(std::format("{}. Are you sure you want to continue? if you continue, The error will be logged and a default json will be used instead.", e.what()));
	}
	try {
		if (!_JsonData.empty() && !_JsonData.is_null()) {
			std::string               logLevelStr(_JsonData["Logging"]["LogLevel"].get<std::string>());
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

			const char* data[21] = { "AMMO", "Arrow", "Bolt", "Sound", "Change Sound Level", "Enable", "Sound Level", "Change Speed", "Change Gravity", "Gravity", "Speed", "Limit Speed", "Limit Damage", "Min",
				"Max", "kLoud", "kNormal", "kSilent", "kVeryLoud", "kQuiet", "Infinite AMMO" };

			_ArrowPatch = _JsonData[data[0]][data[1]]["Enable Arrow Patch"].get<bool>();
			_BoltPatch = _JsonData[data[0]][data[2]]["Enable Bolt Patch"].get<bool>();

			_ChangeArrowSoundLevel = _JsonData[data[0]][data[1]][data[3]][data[4]][data[5]].get<bool>();
			_ArrowSoundLevelStr = _JsonData[data[0]][data[1]][data[3]][data[4]][data[6]].get<std::string>();

			_ChangeBoltSoundLevel = _JsonData[data[0]][data[2]][data[3]][data[4]][data[5]].get<bool>();
			_BoltSoundLevelStr = _JsonData[data[0]][data[2]][data[3]][data[4]][data[6]].get<std::string>();

			_ArrowSpeedEnable = _JsonData[data[0]][data[1]][data[7]][data[5]].get<bool>();
			_BoltSpeedEnable = _JsonData[data[0]][data[2]][data[7]][data[5]].get<bool>();

			_ArrowGravityEnable = _JsonData[data[0]][data[1]][data[8]][data[5]].get<bool>();
			_BoltGravityEnable = _JsonData[data[0]][data[2]][data[8]][data[5]].get<bool>();

			_ArrowGravity = _JsonData[data[0]][data[1]][data[8]][data[9]].get<float>();
			_ArrowSpeed = _JsonData[data[0]][data[1]][data[7]][data[10]].get<float>();

			_BoltGravity = _JsonData[data[0]][data[2]][data[8]][data[9]].get<float>();
			_BoltSpeed = _JsonData[data[0]][data[2]][data[7]][data[10]].get<float>();

			_LimitArrowSpeed = _JsonData[data[0]][data[1]][data[11]][data[5]].get<bool>();
			_LimitBoltSpeed = _JsonData[data[0]][data[2]][data[11]][data[5]].get<bool>();

			_LimitArrowDamage = _JsonData[data[0]][data[1]][data[12]][data[5]].get<bool>();
			_LimitBoltDamage = _JsonData[data[0]][data[2]][data[12]][data[5]].get<bool>();

			_ArrowSpeedLimiterMin = _JsonData[data[0]][data[1]][data[11]][data[13]].get<float>();
			_ArrowSpeedLimiterMax = _JsonData[data[0]][data[1]][data[11]][data[14]].get<float>();

			_BoltSpeedLimiterMin = _JsonData[data[0]][data[2]][data[11]][data[13]].get<float>();
			_BoltSpeedLimiterMax = _JsonData[data[0]][data[2]][data[11]][data[14]].get<float>();

			_ArrowDamageLimiterMin = _JsonData[data[0]][data[1]][data[12]][data[13]].get<float>();
			_ArrowDamageLimiterMax = _JsonData[data[0]][data[1]][data[12]][data[14]].get<float>();

			_BoltDamageLimiterMin = _JsonData[data[0]][data[2]][data[12]][data[13]].get<float>();
			_BoltDamageLimiterMax = _JsonData[data[0]][data[2]][data[12]][data[14]].get<float>();

			_InfinitePlayerAmmo = _JsonData[data[0]][data[20]]["Player"].get<bool>();
			_InfiniteTeammateAmmo = _JsonData[data[0]][data[20]]["Teammate"].get<bool>();

			_Done = true;

			if (_ArrowSoundLevelStr == data[15]) {
				_ArrowSoundLevel = RE::SOUND_LEVEL::kLoud;
			} else if (_ArrowSoundLevelStr == data[16]) {
				_ArrowSoundLevel = RE::SOUND_LEVEL::kNormal;
			} else if (_ArrowSoundLevelStr == data[17]) {
				_ArrowSoundLevel = RE::SOUND_LEVEL::kSilent;
			} else if (_ArrowSoundLevelStr == data[18]) {
				_ArrowSoundLevel = RE::SOUND_LEVEL::kVeryLoud;
			} else if (_ArrowSoundLevelStr == data[19]) {
				_ArrowSoundLevel = RE::SOUND_LEVEL::kQuiet;
			} else {
				logger::error("Invalid Arrow Sound Level specified in the JSON file. Not Patching Arrow Sound Level.");
				_ChangeArrowSoundLevel = false;
			}

			if (_BoltSoundLevelStr == data[15]) {
				_BoltSoundLevel = RE::SOUND_LEVEL::kLoud;
			} else if (_BoltSoundLevelStr == data[16]) {
				_BoltSoundLevel = RE::SOUND_LEVEL::kNormal;
			} else if (_BoltSoundLevelStr == data[17]) {
				_BoltSoundLevel = RE::SOUND_LEVEL::kSilent;
			} else if (_BoltSoundLevelStr == data[18]) {
				_BoltSoundLevel = RE::SOUND_LEVEL::kVeryLoud;
			} else if (_BoltSoundLevelStr == data[19]) {
				_BoltSoundLevel = RE::SOUND_LEVEL::kQuiet;
			} else {
				logger::error("Invalid Bolt Sound Level specified in the JSON file. Not Patching Bolt Sound Level.");
				_ChangeBoltSoundLevel = false;
			}
		}
		if (fs::exists(_FolderPath) && !fs::is_empty(_FolderPath)) {
			_HasFilesToMerge = true;
			for (const auto& entry : fs::directory_iterator(_FolderPath)) {
				if (entry.path().extension() == ".json") {
					std::ifstream jFile(entry.path());
					try {
						_MergeData = nJson::parse(jFile);
					} catch (const nJson::parse_error& e) {
						logger::error("{} parsing error : {}", entry.path().generic_string(), e.what());

						Utils::customMessageBox(std::format("{}. Are you sure you want to continue? if you continue, {} will be ignored and the error will be logged.", e.what(), entry.path().generic_string()));
						continue;
					}
					logger::debug("Loaded JSON from file: {}", entry.path().generic_string());

					const char* data1[2] = { "AMMO FormID to Exclude", "AMMO FormID to Exclude" };

					if (_MergeData[data1[0]].is_array() && _MergeData[data1[1]].is_array()) {
						// Collect all elements into formIDArray
						_FormIDArray.insert(_FormIDArray.end(), _MergeData[data1[0]].begin(), _MergeData[data1[1]].end());

						// Collect all elements into _TESFileArray
						_TESFileArray.insert(_TESFileArray.end(), _MergeData[data1[0]].begin(), _MergeData[data1[1]].end());
					}

					if (!_MergeData.empty() && !_MergeData.is_null())
						_MergeData.clear();
				}
			}

			if (!_FormIDArray.empty()) {
				// Sort and remove duplicates from formIDArray
				std::sort(_FormIDArray.begin(), _FormIDArray.end());
				_FormIDArray.erase(std::unique(_FormIDArray.begin(), _FormIDArray.end()), _FormIDArray.end());
			}

			if (!_TESFileArray.empty()) {
				// Sort and remove duplicates from _TESFileArray
				std::sort(_TESFileArray.begin(), _TESFileArray.end());
				_TESFileArray.erase(std::unique(_TESFileArray.begin(), _TESFileArray.end()), _TESFileArray.end());
			}

			// debug("formIDArray : \n{}",formIDArray.dump(4));
			// debug("_TESFileArray : \n{}",_TESFileArray.dump(4));
		} else
			logger::info("************No Exclusion will be Done************");

	} catch (const nJson::exception& e) {
		logger::error("{}", e.what());
		Utils::customMessageBox(std::format("{}. Want to Continue?", e.what()));
	}
	logger::info("************Finished Processing Data*************");

	static const char* data("*************************************************");

	logger::info("{}", data);
	logger::info("Patch Arrows : {}", _ArrowPatch);
	logger::info("Patch Bolts : {}", _BoltPatch);
	logger::info("{}", data);
	logger::info("Infinite Player AMMO : {}", _InfinitePlayerAmmo);
	logger::info("Infinite Teammate AMMO : {}", _InfiniteTeammateAmmo);
	logger::info("{}", data);
	logger::info("Set Arrow Gravity : {}", _ArrowGravityEnable);
	logger::info("Arrow Gravity : {}", _ArrowGravity);
	logger::info("{}", data);
	logger::info("Set Bolt Gravity : {}", _BoltGravityEnable);
	logger::info("Bolt Gravity : {}", _BoltGravity);
	logger::info("{}", data);
	logger::info("Set Arrow Speed : {}", _ArrowSpeedEnable);
	logger::info("Arrow Speed : {}", _ArrowSpeed);
	logger::info("{}", data);
	logger::info("Set Bolt Speed : {}", _BoltSpeedEnable);
	logger::info("Bolt Speed : {}", _BoltSpeed);
	logger::info("{}", data);
	logger::info("Set Arrow Speed Limit : {}", _LimitArrowSpeed);
	logger::info("Arrow Minimum Speed Limit : {}", _ArrowSpeedLimiterMin);
	logger::info("Arrow Maximum Speed Limit : {}", _ArrowSpeedLimiterMax);
	logger::info("{}", data);
	logger::info("Limit Bolt Speed : {}", _LimitBoltSpeed);
	logger::info("Bolt Minimum Speed Limit : {}", _BoltSpeedLimiterMin);
	logger::info("Bolt Maximum Speed Limit : {}", _BoltSpeedLimiterMax);
	logger::info("{}", data);
	logger::info("Change Arrow Sound Level : {}", _ChangeArrowSoundLevel);
	logger::info("Arrow Sound Level : {}", _ArrowSoundLevelStr);
	logger::info("{}", data);
	logger::info("Change Bolt Sound Level : {}", _ChangeBoltSoundLevel);
	logger::info("Bolt Sound Level : {}", _BoltSoundLevelStr);
	logger::info("{}", data);
	logger::info("Limit Arrow Damage : {}", _LimitArrowDamage);
	logger::info("Arrow Minimum Damage Limit : {}", _ArrowDamageLimiterMin);
	logger::info("Arrow Maximum Damage Limit : {}", _ArrowDamageLimiterMax);
	logger::info("{}", data);
	logger::info("Limit Bolt Damage : {}", _LimitBoltDamage);
	logger::info("Bolt Minimum Damage Limit : {}", _BoltDamageLimiterMin);
	logger::info("Bolt Maximum Damage Limit : {}", _BoltDamageLimiterMax);
	logger::info("{}", data);
}

void DataHandler::ammo_patch()
{
	auto startAP = std::chrono::high_resolution_clock::now();
	if (_ArrowPatch || _BoltPatch) {
		logger::info("{} {} is starting to patch", SKSE::PluginDeclaration::GetSingleton()->GetName(), SKSE::PluginDeclaration::GetSingleton()->GetVersion());
		for (const auto ammo : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESAmmo>()) {
			if (ammo) {
				const char* starString = "******************************************************************************************************************************";
				bool        shouldPatch = true;
				const auto  ammoProjectile = ammo->GetRuntimeData().data.projectile;
				if (ammoProjectile) {
					shouldPatch = true;
					if (_HasFilesToMerge) {
						for (const auto& ammoModName : _TESFileArray) {
							if (ammoModName.c_str() == ammo->GetFile()->GetFilename()) {
								shouldPatch = false;
								logger::debug("{}", starString);
								logger::debug("From {} :", ammoModName.c_str());
								logger::debug("Skipping Ammo : Name:{}|FormID:{:08X}|Damage:{}|Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}", ammo->GetFullName(), ammo->GetRawFormID(),
									ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed, ammoProjectile->data.gravity);
								logger::debug("{}", starString);
								break;
							}
						}
						if (shouldPatch) {
							for (const auto& ammoFormID : _FormIDArray) {
								if (auto form = Utils::GetFormFromIdentifier(ammoFormID); form)
									if (ammo->GetFormID() == form) {
										shouldPatch = false;
										logger::debug("{}", starString);
										logger::debug("Skipping Ammo : Name:{}|FormID:{:08X}|Damage:{}|Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
											ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed, ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
										logger::debug("{}", starString);
										break;
									}
							}
						}
					}
					if (shouldPatch) {
						if (!(ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonPlayable)) {
							bool ammoPatched = false;
							if (_ChangeArrowSoundLevel || _ArrowSpeedEnable || _ArrowGravityEnable || _LimitArrowDamage || _LimitArrowSpeed || _ChangeBoltSoundLevel || _BoltSpeedEnable || _BoltGravityEnable || _LimitBoltDamage || _LimitBoltSpeed)
								ammoPatched = true;
							if ((_ArrowPatch || _BoltPatch) && ammoPatched) {
								logger::debug("{}", starString);
								logger::debug("Before Patching : Name:{}|FormID:{:08X}|Damage:{}|Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
									ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed, ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
							}

							if (ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt) {  // for arrow
								if (_ArrowPatch) {
									if (_ChangeArrowSoundLevel) {  // set sound level
										ammoProjectile->soundLevel = _ArrowSoundLevel;
										logger::debug("changed Arrow Sound Level");
									}
									if (_ArrowSpeedEnable) {  // set speed
										ammoProjectile->data.speed = _ArrowSpeed;
										logger::debug("Changed Arrow Speed");
									}
									if (_ArrowGravityEnable) {  // set gravity
										ammoProjectile->data.gravity = _ArrowGravity;
										logger::debug("Changed Arrow Gravity");
									}
									if (_LimitArrowDamage) {  // limit damage
										ammo->GetRuntimeData().data.damage = InlineUtils::limitFloat(ammo->GetRuntimeData().data.damage, _ArrowDamageLimiterMin, _ArrowDamageLimiterMax);
										logger::debug("Limited Arrow Damage");
									}
									if (_LimitArrowSpeed) {  // limit speed
										ammoProjectile->data.speed = InlineUtils::limitFloat(ammoProjectile->data.speed, _ArrowSpeedLimiterMin, _ArrowSpeedLimiterMax);
										logger::debug("Limited Arrow Level");
									}
								}
							}

							if (!(ammo->GetRuntimeData().data.flags & RE::AMMO_DATA::Flag::kNonBolt)) {  // for bolt
								if (_BoltPatch) {
									if (_ChangeBoltSoundLevel) {  // set sound level of bolt
										ammoProjectile->soundLevel = _BoltSoundLevel;
										logger::debug("changed Bolt Sound Level");
									}
									if (_BoltSpeedEnable) {  // set speed of bolt
										ammoProjectile->data.speed = _BoltSpeed;
										logger::debug("Changed Bolt Speed");
									}
									if (_BoltGravityEnable) {  // set gravity of bolt
										ammoProjectile->data.gravity = _BoltGravity;
										logger::debug("Changed Bolt Speed");
									}
									if (_LimitBoltSpeed) {  // limit speed of bolt
										ammoProjectile->data.speed = InlineUtils::limitFloat(ammoProjectile->data.speed, _BoltSpeedLimiterMin, _BoltSpeedLimiterMax);
										logger::debug("Limited Bolt Speed");
									}
									if (_LimitBoltDamage) {  // limit damage of bolt
										ammo->GetRuntimeData().data.damage = InlineUtils::limitFloat(ammo->GetRuntimeData().data.damage, _BoltDamageLimiterMin, _BoltDamageLimiterMax);
										logger::debug("Limited Bolt Damage");
									}
								}
							}

							if ((_ArrowPatch || _BoltPatch) && ammoPatched) {
								logger::debug("After Patching : Name:{}|FormID:{:08X}|Damage:{}|Projectile Name:{}|Projectile FormID:{:08X}|Projectile Speed:{}|Projectile Gravity:{}|File:{}", ammo->GetFullName(), ammo->GetRawFormID(),
									ammo->GetRuntimeData().data.damage, ammoProjectile->GetFullName(), ammoProjectile->GetRawFormID(), ammoProjectile->data.speed, ammoProjectile->data.gravity, ammo->GetFile()->GetFilename());
								logger::debug("{}", starString);
							}
						}
					}
				} else
					logger::info("PROJ Record for {} with FormID {:08X} from file {} is NULL", ammo->GetFullName(), ammo->GetRawFormID(), ammo->GetFile()->GetFilename());
			} else
				logger::debug("This Iteration's Ammo is nullptr");
		}
		logger::info("{} {} has finished Patching", SKSE::PluginDeclaration::GetSingleton()->GetName(), SKSE::PluginDeclaration::GetSingleton()->GetVersion());
		if (!_FormIDArray.empty())
			_FormIDArray.clear();
		if (!_TESFileArray.empty())
			_TESFileArray.clear();
	} else
		logger::info("Not Patching");
	auto nanosecondsTakenForAP = std::chrono::duration(std::chrono::high_resolution_clock::now() - startAP);
	logger::info("Time Taken in ammo_patch() totally is {} nanoseconds or {} microseconds or {} milliseconds or {} seconds or {} minutes", nanosecondsTakenForAP.count(),
		std::chrono::duration_cast<std::chrono::microseconds>(nanosecondsTakenForAP).count(), std::chrono::duration_cast<std::chrono::milliseconds>(nanosecondsTakenForAP).count(),
		std::chrono::duration_cast<std::chrono::seconds>(nanosecondsTakenForAP).count(), std::chrono::duration_cast<std::chrono::minutes>(nanosecondsTakenForAP).count());
}
