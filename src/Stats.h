#pragma once
#include <set>
#include <string>
#include <Glacier/Enums.h>
#include "PlayStyleRating.h"

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

struct KillMethodStats
{
	int accident = 0;
	int headshot = 0;
	int melee = 0;
	int thrown = 0;
	int pistol = 0;
	int silencedWeapon = 0;
};

struct KillStats
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
	bool allHidden = false;
	bool allTargetsHidden = false;
	int hidden = 0;
	int found = 0;
	int foundAccidents = 0;
	int foundMurdered = 0;
	int foundCrowd = 0;
	int foundCrowdMurders = 0;
	int deadSeen = 0;
	int targetsFound = 0;
	int bagged = 0;
};

struct DetectionStats
{
	bool onCamera = false;
	int spotted = 0;
	int caughtTrespassing = 0;
	int targetsSpottedBy = 0;
	int targetsSpottedByAndKilled = 0;
	int nonTargetsSpottedBy = 0;
	int uniqueNPCsCaughtByAndKilled = 0;
	int witnesses = 0;
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
	bool suitRetrieved = false;
	bool recorderErased = false;
	bool recorderDestroyed = false;
	bool recordedThenErased = false;
	int agilityActions = 0;
	int camerasDestroyed = 0;
	int disguisesTaken = 0;
	int disguisesBlown = 0;
	int doorsUnlocked = 0;
	int itemsPickedUp = 0;
	int itemsRemovedFromInventory = 0;
	int itemsThrown = 0;
	int shotsFired = 0;
	int missedShots = 0;
	int setpiecesDestroyed = 0;
	int targetsMadeSick = 0;
	int timesTrespassed = 0;
};

struct CurrentStats
{
	EGameTension tension = EGameTension::EGT_Undefined;
	bool trespassing = false;
};

struct Stats
{
	std::set<std::string> witnesses;
	std::set<std::string> spottedBy;
	std::set<std::string> targetsSpottedBy;
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
