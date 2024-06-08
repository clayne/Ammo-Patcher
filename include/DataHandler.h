#pragma once

class DataHandler
{
public:
	static DataHandler* GetSingleton();

	void LoadJson();

	void ammo_patch();

	bool OptionInfiniteArrows() { return _InfinitePlayerAmmo; }

private:
	DataHandler() = default;
	~DataHandler() = default;
	DataHandler(const DataHandler&) = delete;
	DataHandler(DataHandler&&) = delete;
	DataHandler& operator=(const DataHandler&) = delete;
	DataHandler& operator=(DataHandler&&) = delete;

	bool                     _ArrowPatch{ true };
	bool                     _BoltPatch{ true };
	bool                     _ArrowSpeedEnable{ true };
	bool                     _BoltSpeedEnable{ true };
	bool                     _ArrowGravityEnable{ true };
	bool                     _BoltGravityEnable{ true };

public:
	bool                     _InfinitePlayerAmmo{ false };
	bool                     _InfiniteTeammateAmmo{ false };

private:
	bool                     _LimitArrowSpeed{ false };
	bool                     _LimitBoltSpeed{ false };
	bool                     _LimitArrowDamage{ false };
	bool                     _LimitBoltDamage{ false };
	bool                     _ChangeArrowSoundLevel{ false };
	bool                     _ChangeBoltSoundLevel{ false };
	bool                     _HasFilesToMerge{ false };
	float                    _ArrowDamageLimiterMin{ 10.0f };
	float                    _ArrowDamageLimiterMax{ 1000.0f };
	float                    _BoltDamageLimiterMin{ 10.0f };
	float                    _BoltDamageLimiterMax{ 1000.0f };
	float                    _ArrowSpeedLimiterMin{ 3000.0f };
	float                    _ArrowSpeedLimiterMax{ 12000.0f };
	float                    _BoltSpeedLimiterMin{ 4000.0f };
	float                    _BoltSpeedLimiterMax{ 12000.0f };
	float                    _ArrowSpeed{ 9000.0f };
	float                    _ArrowGravity{ 0.0f };
	float                    _BoltSpeed{ 10800.0f };
	float                    _BoltGravity{ 0.0f };
	std::string              _ArrowSoundLevelStr{ "kSilent" };
	std::string              _BoltSoundLevelStr{ "kSilent" };
	RE::SOUND_LEVEL          _ArrowSoundLevel{ RE::SOUND_LEVEL::kSilent };
	RE::SOUND_LEVEL          _BoltSoundLevel{ RE::SOUND_LEVEL::kSilent };
	const char*              _FolderPath{ "Data/SKSE/Plugins/Ammo Patcher/" };
	nJson                    _JsonData;   // used for main json file
	nJson                    _MergeData;  // used to merge exclusion json files
	std::vector<std::string> _FormIDArray;
	std::vector<std::string> _TESFileArray;
};
