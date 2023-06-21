#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <Glacier/Enums.h>
#include <Glacier/ZMath.h>
#include "json.hpp"
#include "Enums.h"
#include "EventSystem.h"

struct GameChanger {
	//std::string Id;
	//std::string Name;
	//std::string Description;
	//std::optional<std::string> TileImage;
	//std::optional<std::string> Icon;
	//std::optional<std::string> ObjectivesCategory;
	//std::optional<bool> IsHidden;
	//std::optional<std::vector<std::string>> Resource;
	//std::optional<std::vector<MissionManifestObjective>> Objectives;
	//std::optional<std::string> LongDescription
	//std::optional<bool> IsPrestigeObjective;
};

struct LoadoutItemEventValue {
	std::string RepositoryId;
	std::string InstanceId;
	std::vector<std::string> OnlineTraits;
	std::optional<std::nullptr_t> Category;
};

struct DamageHistoryEventValue {
	bool Explosive;
	bool Headshot;
	bool Accident;
	bool WeaponSilenced;
	bool Projectile;
	bool Sniper;
	bool ThroughWall;
	std::string InstanceId;
	std::string RepositoryId;
	int BodyPartId;
	int TotalDamage;

	DamageHistoryEventValue(const nlohmann::json& json) :
		Explosive(json.value("Explosive", false)),
		Headshot(json.value("Headshot", false)),
		Accident(json.value("Accident", false)),
		WeaponSilenced(json.value("WeaponSilenced", false)),
		Projectile(json.value("Projectile", false)),
		Sniper(json.value("Sniper", false)),
		ThroughWall(json.value("ThroughWall", false)),
		InstanceId(json.value("InstanceId", "")),
		RepositoryId(json.value("RepositoryId", "")),
		BodyPartId(json.value("BodyPartId", 0)),
		TotalDamage(json.value("TotalDamage", 0))
	{ }
};

struct PacifyEventValue {
	std::string RepositoryId;
	uint32_t ActorId;
	std::string ActorName;
	EActorType ActorType;
	EKillType KillType;
	EDeathContext KillContext;
	std::string KillClass;
	bool Accident;
	bool WeaponSilenced;
	bool Explosive;
	int ExplosionType;
	bool Projectile;
	bool Sniper;
	bool IsHeadshot;
	bool IsTarget;
	bool ThroughWall;
	int BodyPartId;
	double TotalDamage;
	bool IsMoving;
	int RoomId;
	std::string ActorPosition;
	std::string HeroPosition;
	std::vector<std::string> DamageEvents;
	int PlayerId;
	std::string OutfitRepositoryId;
	bool OutfitIsHitmanSuit;
	std::string KillMethodBroad;
	std::string KillMethodStrict;
	int EvergreenRarity;
	std::vector<DamageHistoryEventValue> History;

	PacifyEventValue(const nlohmann::json& json) :
		RepositoryId(json.value("RepositoryId", "")),
		ActorId(json.value("ActorId", 0)),
		ActorName(json.value("ActorName", "")),
		ActorType(getActorTypeFromValue(json.value("ActorType", 0))),
		KillType(getKillTypeFromValue(json.value("KillType", 0))),
		KillContext(getDeathContextFromValue(json.value("KillContext", 0))),
		KillClass(json.value("KillClass", "")),
		Accident(json.value("Accident", false)),
		WeaponSilenced(json.value("WeaponSilenced", false)),
		Explosive(json.value("Explosive", false)),
		ExplosionType(json.value("ExplosionType", 0)),
		Projectile(json.value("Projectile", false)),
		Sniper(json.value("Sniper", false)),
		IsHeadshot(json.value("IsHeadshot", false)),
		IsTarget(json.value("IsTarget", false)),
		ThroughWall(json.value("ThroughWall", false)),
		BodyPartId(json.value("BodyPartId", -1)),
		TotalDamage(json.value("TotalDamage", 0)),
		IsMoving(json.value("IsMoving", false)),
		RoomId(json.value("RoomId", -1)),
		ActorPosition(json.value("ActorPosition", "")),
		HeroPosition(json.value("HeroPosition", "")),
		DamageEvents(json["DamageEvents"].get<std::vector<std::string>>()),
		PlayerId(json.value("PlayerId", -1)),
		OutfitRepositoryId(json.value("OutfitRepositoryId", "")),
		OutfitIsHitmanSuit(json.value("OutfitIsHitmanSuit", false)),
		KillMethodBroad(json.value("KillMethodBroad", "")),
		KillMethodStrict(json.value("KillMethodStrict", "")),
		EvergreenRarity(json.value("EvergreenRarity", -1))
	{
		auto& history = json["History"];
		if (history.is_array()) {
			for (auto& historyIt : history)
				this->History.emplace_back(historyIt);
		}
	}
};

struct KillEventValue : PacifyEventValue {
	std::string KillItemRepositoryId;
	std::string KillItemInstanceId;
	std::string KillItemCategory;
};

struct VoidEventValue {
	VoidEventValue(const nlohmann::json& json) {}
};

struct StringEventValue {
	std::string value;

	StringEventValue(const nlohmann::json& json) : value(json.get<std::string>())
	{ }
};

struct StringArrayEventValue {
	std::vector<std::string> value;

	StringArrayEventValue(const nlohmann::json& json) {
		if (!json.is_array()) return;
		for (auto& v : json) {
			this->value.emplace_back(v.get<std::string>());
		}
	}
};

struct TakedownCleannessEventValue {
	std::string RepositoryId;
	bool IsTarget;

	TakedownCleannessEventValue(const nlohmann::json& json) :
		RepositoryId(json.value("RepositoryId", "")),
		IsTarget(json.value("IsTarget", false))
	{ }
};

struct ActorIdentityEventValue {
	unsigned ActorId;
	std::string RepositoryId;
	std::string ActorName;

	ActorIdentityEventValue(const nlohmann::json& json) :
		ActorId(json.value("ActorId", 0)),
		RepositoryId(json.value("RepositoryId", "")),
		ActorName(json.value("ActorName", ""))
	{ }
};

struct BodyEventValue {
	std::string RepositoryId;
	bool IsCrowdActor;

	BodyEventValue(const nlohmann::json& json) :
		RepositoryId(json.value("RepositoryId", "")),
		IsCrowdActor(json.value("IsCrowdActor", false))
	{ }
};

struct BodyKillInfoEventValue : BodyEventValue {
	EDeathContext DeathContext;
	EDeathType DeathType;

	BodyKillInfoEventValue(const nlohmann::json& json) :
		BodyEventValue(json),
		DeathContext(getDeathContextFromValue(json.value("DeathContext", 0))),
		DeathType(getDeathTypeFromValue(json.value("DeathType", 0)))
	{ }
};

struct ItemEventValue {
	std::string RepositoryId;
	std::string ItemType;
	std::string ItemName;
	//std::vector<std::string> OnlineTraits;
	//std::string ActionRewardType;

	ItemEventValue(const nlohmann::json& json) {
		ItemName = json.value("ItemName", "");
		ItemType = json.value("ItemType", "");
		RepositoryId = json.value("RepositoryId", "");
	}
};

template<>
struct Event<Events::ContractStart> {
	static auto constexpr Name = "ContractStart";

	struct EventValue {
		std::vector<LoadoutItemEventValue> Loadout;
		std::string Disguise;
		std::string LocationId;
		MissionType ContractType;
		std::vector<GameChanger> GameChangers;
		int DifficultyLevel;
		bool IsVR;
		bool IsHitmanSuit;
		std::string SelectedCharacterId;
		int EvergreenSeed;
		int EvergreenDifficulty;

		EventValue(const nlohmann::json& value) {
			auto& loadout = value["Loadout"];
			for (auto const& ld : loadout)
			{
				LoadoutItemEventValue loadoutItem;
				loadoutItem.RepositoryId = ld.value("RepositoryId", "");
				loadoutItem.InstanceId = ld.value("RepositoryId", "");
				loadoutItem.OnlineTraits = ld["OnlineTraits"].get<std::vector<std::string>>();
				loadoutItem.Category = nullptr;
				Loadout.emplace_back(loadoutItem);
			}
			Disguise = value.value("Disguise", "");
			LocationId = value.value("LocationId", "");
			ContractType = *getMissionTypeFromString(value.value("ContractType", ""));
			DifficultyLevel = value.value<int>("DifficultyLevel", -1);
			IsVR = value.value<bool>("IsVR", false);
			IsHitmanSuit = value.value<bool>("IsHitmanSuit", false);
			SelectedCharacterId = value.value("SelectedCharacterId", "");
			EvergreenSeed = value.value("EvergreenSeed", 0);
			EvergreenDifficulty = value.value("EvergreenDifficulty", 0);
		}
	};
};

template<>
struct Event<Events::ContractEnd> {
	static auto constexpr Name = "ContractEnd";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::IntroCutEnd> {
	static auto constexpr Name = "IntroCutEnd";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::setpieces> {
	static auto constexpr Name = "setpieces";
	struct EventValue {
		std::string RepositoryId;
		std::string name_metricvalue;
		std::string setpieceHelper_metricvalue;
		std::string setpieceType_metricvalue;
		std::string toolUsed_metricvalue;
		std::string Item_triggered_metricvalue;
		SVector3 Position;

		EventValue(const nlohmann::json& json) :
			RepositoryId(json.value("RepositoryId", "")),
			name_metricvalue(json.value("name_metricvalue", "")),
			setpieceHelper_metricvalue(json.value("setpieceHelper_metricvalue", "")),
			setpieceType_metricvalue(json.value("setpieceType_metricvalue", "")),
			toolUsed_metricvalue(json.value("toolUsed_metricvalue", "")),
			Item_triggered_metricvalue(json.value("Item_triggered_metricvalue", "")),
			Position(json.value("x", 0.0), json.value("y", 0.0), json.value("z", 0.0))
		{ }
	};
};

template<>
struct Event<Events::AddSyndicateTarget> {
	static auto constexpr Name = "AddSyndicateTarget";
	struct EventValue {
		std::string repoID;

		EventValue(const nlohmann::json& json) {
			repoID = json.value("repoID", "");
		}
	};
};

template<>
struct Event<Events::StartingSuit> {
	static auto constexpr Name = "StartingSuit";
	using EventValue = StringEventValue;
};

template<>
struct Event<Events::ItemPickedUp> {
	static auto constexpr Name = "ItemPickedUp";
	using EventValue = ItemEventValue;
};

template<>
struct Event<Events::ItemRemovedFromInventory> {
	static auto constexpr Name = "ItemRemovedFromInventory";
	using EventValue = ItemEventValue;
};

template<>
struct Event<Events::ItemDropped> {
	static auto constexpr Name = "ItemDropped";
	using EventValue = ItemEventValue;
};

template<>
struct Event<Events::ItemThrown> {
	static auto constexpr Name = "ItemThrown";
	using EventValue = ItemEventValue;
};

template<>
struct Event<Events::Disguise> {
	static auto constexpr Name = "Disguise";
	using EventValue = StringEventValue;
};

template<>
struct Event<Events::SituationContained> {
	static auto constexpr Name = "SituationContained";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::Actorsick> {
	static auto constexpr Name = "Actorsick";
	struct EventValue {
		SVector3 ActorPosition;
		unsigned ActorId;
		std::string ActorName;
		std::string actor_R_ID;
		bool IsTarget;
		std::string item_R_ID;
		std::string setpiece_R_ID;
		EActorType ActorType;

		EventValue(const nlohmann::json& json) :
			ActorPosition(json.value("x", 0.0), json.value("y", 0.0), json.value("z", 0.0)),
			ActorId(json.value("ActorId", 0)),
			ActorName(json.value("ActorName", "")),
			actor_R_ID(json.value("actor_R_ID", "")),
			IsTarget(json.value("IsTarget", false)),
			item_R_ID(json.value("item_R_ID", "")),
			setpiece_R_ID(json.value("setpiece_R_ID", "")),
			ActorType(getActorTypeFromValue(json.value("ActorType", -1)))
		{ }
	};
};

template<>
struct Event<Events::Dart_Hit> {
	static auto constexpr Name = "Dart_Hit";
	struct EventValue {
		std::string RepositoryId;
		EActorType ActorType;
		bool IsTarget;
		bool Blind;
		bool Sedative;
		bool Sick;

		EventValue(const nlohmann::json& json) :
			RepositoryId(json.value("RepositoryId", "")),
			ActorType(getActorTypeFromValue(json.value("ActorType", -1))),
			IsTarget(json.value("IsTarget", false)),
			Blind(json.find("Blind") != json.end()),
			Sedative(json.find("Sedative") != json.end()),
			Sick(json.find("Sick") != json.end())
		{ }
	};
};

template<>
struct Event<Events::Trespassing> {
	static auto constexpr Name = "Trespassing";
	struct EventValue {
		bool IsTrespassing;
		int RoomId;

		EventValue(const nlohmann::json& json) :
			IsTrespassing(json.value("IsTrespassing", false)),
			RoomId(json.value("RoomId", -1))
		{ }
	};
};

template<>
struct Event<Events::SecuritySystemRecorder> {
	static auto constexpr Name = "SecuritySystemRecorder";
	struct EventValue {
		SecuritySystemRecorderEvent event;
		unsigned camera;
		unsigned recorder;

		EventValue(const nlohmann::json& json) :
			event(getSecuritySystemRecorderEventFromString(json.value("event", ""))),
			camera(json.value("camera", 0)),
			recorder(json.value("recorder", 0))
		{ }
	};
};

template<>
struct Event<Events::Agility_Start> {
	static auto constexpr Name = "Agility_Start";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::Drain_Pipe_Climbed> {
	static auto constexpr Name = "Drain_Pipe_Climbed";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::ExitGate> {
	static auto constexpr Name = "exit_gate";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::AccidentBodyFound> {
	static auto constexpr Name = "AccidentBodyFound";
	struct EventValue {
		BodyKillInfoEventValue DeadBody;

		EventValue(const nlohmann::json& json) : DeadBody(json["DeadBody"])
		{}
	};
};

template<>
struct Event<Events::BodyFound> {
	static auto constexpr Name = "BodyFound";
	struct EventValue {
		BodyKillInfoEventValue DeadBody;

		EventValue(const nlohmann::json& json) : DeadBody(json["DeadBody"])
		{}
	};
};

template<>
struct Event<Events::DeadBodySeen> {
	static auto constexpr Name = "DeadBodySeen";
	using EventValue = StringEventValue;
};

template<>
struct Event<Events::MurderedBodySeen> {
	static auto constexpr Name = "MurderedBodySeen";
	struct EventValue {
		BodyEventValue DeadBody;
		std::string Witness;
		bool IsWitnessTarget;

		EventValue(const nlohmann::json& json) : DeadBody(json["DeadBody"]),
			Witness(json.value("Witness", "")),
			IsWitnessTarget(json.value("IsWitnessTarget", false))
		{}
	};
};

template<>
struct Event<Events::NoticedKill> {
	static auto constexpr Name = "NoticedKill";
	using EventValue = TakedownCleannessEventValue;
};

template<>
struct Event<Events::Noticed_Pacified> {
	static auto constexpr Name = "Noticed_Pacified";
	using EventValue = TakedownCleannessEventValue;
};

template<>
struct Event<Events::Unnoticed_Kill> {
	static auto constexpr Name = "Unnoticed_Kill";
	using EventValue = TakedownCleannessEventValue;
};

template<>
struct Event<Events::Unnoticed_Pacified> {
	static auto constexpr Name = "Unnoticed_Pacified";
	using EventValue = TakedownCleannessEventValue;
};

template<>
struct Event<Events::AllBodiesHidden> {
	static auto constexpr Name = "AllBodiesHidden";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::FirstMissedShot> {
	static auto constexpr Name = "FirstMissedShot";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::FirstNonHeadshot> {
	static auto constexpr Name = "FirstNonHeadshot";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::TargetBodySpotted> {
	static auto constexpr Name = "TargetBodySpotted";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::BodyBagged> {
	static auto constexpr Name = "BodyBagged";
	using EventValue = ActorIdentityEventValue;
};

template<>
struct Event<Events::BodyHidden> {
	static auto constexpr Name = "BodyHidden";
	using EventValue = ActorIdentityEventValue;
};

template<>
struct Event<Events::ShotsFired> {
	static auto constexpr Name = "ShotsFired";
	struct EventValue {
		int Split;
		int Total;

		EventValue(const nlohmann::json& json) : Split(json["Split"].value("", 0)),
			Total(json.value("Total", 0))
		{}
	};
};

template<>
struct Event<Events::Door_Unlocked> {
	static auto constexpr Name = "Door_Unlocked";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::CrowdNPC_Died> {
	static auto constexpr Name = "CrowdNPC_Died";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::TargetEliminated> {
	static auto constexpr Name = "TargetEliminated";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::_47_FoundTrespassing> {
	static auto constexpr Name = "47_FoundTrespassing";
	using EventValue = VoidEventValue;
};

template<>
struct Event<Events::Spotted> {
	static auto constexpr Name = "Spotted";
	using EventValue = StringArrayEventValue;
};

template<>
struct Event<Events::Witnesses> {
	static auto constexpr Name = "Witnesses";
	using EventValue = StringArrayEventValue;
};

template<>
struct Event<Events::DisguiseBlown> {
	static auto constexpr Name = "DisguiseBlown";
	using EventValue = StringEventValue;
};

template<>
struct Event<Events::BrokenDisguiseCleared> {
	static auto constexpr Name = "BrokenDisguiseCleared";
	using EventValue = StringEventValue;
};

template<>
struct Event<Events::AmbientChanged> {
	static auto constexpr Name = "AmbientChanged";
	struct EventValue {
		EGameTension PreviousAmbientValue;
		EGameTension AmbientValue;
		//std::string PreviousAmbient;
		//std::string Ambient;

		EventValue(const nlohmann::json& json) :
			PreviousAmbientValue(getGameTensionFromValue(json.value("PreviousAmbientValue", 0))),
			AmbientValue(getGameTensionFromValue(json.value("PreviousAmbientValue", 0)))
			//PreviousAmbient(json.value("PreviousAmbient", "")),
			//Ambient(json.value("Ambient", "")),
		{}
	};
};

template<>
struct Event<Events::HoldingIllegalWeapon> {
	static auto constexpr Name = "HoldingIllegalWeapon";
	struct EventValue {
		bool IsHoldingIllegalWeapon;

		EventValue(const nlohmann::json& json) :
			IsHoldingIllegalWeapon(json.value("IsHoldingIllegalWeapon", false))
		{}
	};
};

template<>
struct Event<Events::Pacify> {
	static auto constexpr Name = "Pacify";
	using EventValue = PacifyEventValue;
};

template<>
struct Event<Events::Kill> {
	static auto constexpr Name = "Kill";
	using EventValue = KillEventValue;
};

template<>
struct Event<Events::OpportunityEvents> {
	struct EventValue {
		std::string RepositoryId;
		std::string Event;
	};
};
