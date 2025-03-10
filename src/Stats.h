#pragma once
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <Glacier/Enums.h>
#include "Enums.h"
#include "PlayStyleRating.h"
#include "util.h"

enum class SilentAssassinStatus
{
	OK,
	Fail,
	RedeemableCamera,
	RedeemableTarget,
	RedeemableCameraAndTarget,
};

struct PlayStyle
{
	const PlayStyleRating* rating = nullptr;
	size_t index = 0;
};

struct DisplayStats
{
	int guardKills = 0;
	int civilianKills = 0;
	int noticedKills = 0;
	int tension = 0;
	int pacifications = 0;
	int spotted = 0;
	int witnesses = 0;
	int bodiesHidden = 0;
	int bodiesFound = 0;
	int disguisesTaken = 0;
	int disguisesBlown = 0;
	bool recorded = false;
	bool targetsFound = false;
	PlayStyle playstyle;
	double stealthRating = 0;
	SilentAssassinStatus silentAssassin = SilentAssassinStatus::OK;
};

enum class ItemInfoType {
	None,
	Key,
	Intel,
	Detonator,
	Coin,
	Firearm,
	AmmoBox,
	Explosive,
	Melee,
	LethalMelee,
	Poison,
	Briefcase,
	Other,
};

struct ItemInfo
{
	ItemInfoType type = ItemInfoType::None;
	std::string name;
	std::string commonName;
	std::string itemType;
	std::string inventoryCategoryIcon;
	int count = 1;
};

struct KillMethodStats
{
	int accident = 0;
	int accidentTarget = 0;
	int headshot = 0;
	int headshotTarget = 0;
	int melee = 0;
	int meleeTarget = 0;
	int unarmed = 0;
	int unarmedTarget = 0;
	int thrown = 0;
	int thrownTarget = 0;
	int pistol = 0;
	int pistolTarget = 0;
	int smg = 0;
	int smgTarget = 0;
	int shotgun = 0;
	int shotgunTarget = 0;
	int pistolElim = 0;
	int pistolElimTarget = 0;
	int silencedWeapon = 0;
	int silencedWeaponTarget = 0;

	int drown = 0;
	int drownTarget = 0;
	int push = 0;
	int pushTarget = 0;
	int burn = 0;
	int burnTarget = 0;
	int accidentExplosion = 0;
	int accidentExplosionTarget = 0;
	int fallingObject = 0;
	int fallingObjectTarget = 0;
};

struct KillStats
{
	struct NoticedKillInfo
	{
		std::map<std::string, bool, InsensitiveCompareLexicographic> sightings;
		bool isSightedByNonTarget = false;
	};

	std::set<std::string, InsensitiveCompareLexicographic> targets;
	std::set<std::string, InsensitiveCompareLexicographic> nonTargets;
	std::set<std::string, InsensitiveCompareLexicographic> proxyDeaths;
	std::unordered_map<std::string, NoticedKillInfo, StringHashLowercase, InsensitiveCompare> noticedKillInfos;
	int total = 0;
	int noticed = 0;
	int unnoticed = 0;
	int unnoticedTarget = 0;
	int unnoticedNonTarget = 0;
	int guard = 0;
	int civilian = 0;
	int crowd = 0;
};

struct PacificationStats
{
	int total = 0;
	int targets = 0;
	int nonTargets = 0;
	int noticed = 0;
	int unnoticed = 0;
	int unnoticedNonTarget = 0;
	int guard = 0;
	int civilian = 0;
};

struct PacificationMethodStats
{
	int accident = 0;
	int melee = 0;
	int thrown = 0;
};

struct BodyStats
{
	struct MurderedBodyFoundInfo
	{
		std::map<std::string, bool, InsensitiveCompareLexicographic> sightings;
		bool isSightedByNonTarget = false;
	};

	std::set<std::string, InsensitiveCompareLexicographic> uniqueBodiesFound;
	std::unordered_map<std::string, MurderedBodyFoundInfo, StringHashLowercase, InsensitiveCompare> foundMurderedInfos;
	bool allHidden = false;
	bool allTargetsHidden = false;
	int hidden = 0;
	int found = 0;
	int foundAccidents = 0;
	int foundMurdered = 0;
	int foundMurderedByNonTarget = 0;
	int foundCrowd = 0;
	int foundCrowdMurders = 0;
	int targetBodyWitnessesKilled = 0;
	int deadSeen = 0;
	int targetsFound = 0;
	int bagged = 0;
};

struct DetectionStats
{
	bool onCamera = false;
	int spotted = 0;
	int caughtTrespassing = 0;
	int targetsSpottedByAndKilled = 0;
	int nonTargetsSpottedBy = 0;
	int uniqueNPCsCaughtByAndKilled = 0;
	int witnessesKilled = 0;
	int situationsContained = 0;
};

struct TensionStats
{
	int alertedLow = 0;
	int alertedHigh = 0;
	int agitated = 0;
	int searching = 0;
	int hunting = 0;
	int arrest = 0;
	int combat = 0;
	int level = 0;
};

struct MiscStats
{
	bool startedInSuit = false;
	bool suitRetrieved = false;
	bool recorderErased = false;
	bool recorderDestroyed = false;
	bool recordedThenErased = false;
	int agilityActions = 0;
	int camerasDestroyed = 0;
	int closeCombatEngagements = 0;
	int disguisesTaken = 0;
	int doorsUnlocked = 0;
	int intelItemsPickedUp = 0;
	int itemsPickedUp = 0;
	int itemsRemovedFromInventory = 0;
	int itemsDropped = 0;
	int itemsThrown = 0;
	int keyItemsPickedUp = 0;
	int missedShots = 0;
	int objectsDestroyed = 0;
	int setpiecesDestroyed = 0;
	int shotsFired = 0;
	int targetsMadeSick = 0;
	int timesTrespassed = 0;
	double trespassTime = 0;
	double weaponHoldingTime = 0;
};

struct CurrentStats
{
	EGameTension tension = EGameTension::EGT_Undefined;
	bool inSuit = false;
	bool trespassing = false;
	bool disguiseBlown = false;
	bool holdingIllegalWeapon = false;
};

struct Stats
{
	struct WitnessEvent {
		double timestamp;
		Events event;
		std::string bodyId;
		std::string witnessId;
		bool isWitnessTarget;

		WitnessEvent(double timestamp, Events event, std::string witnessId, bool isWitnessTarget, std::string bodyId = "") :
			timestamp(timestamp), event(event), witnessId(witnessId), bodyId(bodyId), isWitnessTarget(isWitnessTarget)
		{}
	};

	double trespassStartTime = 0;
	double currentTrespassTime = 0;
	double weaponHoldingStartTime = 0;
	std::vector<WitnessEvent> witnessEvents;
	std::string firstSpottedByID;
	std::string firstSpottedByName;
	std::string firstBodyFoundByID;
	std::string firstBodyFoundByName;
	std::string firstNTKID;
	std::string firstNTKName;
	bool firstBodyFoundWasByTarget = false;
	std::set<std::string, InsensitiveCompareLexicographic> witnesses;
	std::set<std::string, InsensitiveCompareLexicographic> spottedBy;
	std::set<std::string, InsensitiveCompareLexicographic> targetsSpottedBy;
	std::set<std::string, InsensitiveCompareLexicographic> targetBodyWitnesses;
	std::set<std::string, InsensitiveCompareLexicographic> targetKillNoticers;
	std::set<std::string, InsensitiveCompareLexicographic> disguisesBlown;
	std::map<std::string, ItemInfo, InsensitiveCompareLexicographic> itemsObtained;
	std::map<std::string, ItemInfo, InsensitiveCompareLexicographic> itemsDisposed;
	KillStats kills;
	KillMethodStats killMethods;
	PacificationStats pacifies;
	PacificationMethodStats pacifyMethods;
	BodyStats bodies;
	DetectionStats detection;
	TensionStats tension;
	CurrentStats current;
	MiscStats misc;
};
