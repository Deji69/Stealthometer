#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <Glacier/Enums.h>

enum class MissionType {
	Arcade,
	Campaign,
	Creation,
	Elusive,
	Escalation,
	Evergreen,
	Featured,
	Mission,
	Orbis,
	Sniper,
	Tutorial,
	UserCreated,
	VsRace,
};

enum class Events {
	_47_FoundTrespassing,
	AmbientChanged,
	AccidentBodyFound,
	Actorsick,
	AddSyndicateTarget,
	Agility_Start,
	AllBodiesHidden,
	BodyBagged,
	BodyFound,
	BodyHidden,
	BrokenDisguiseCleared,
	ContractStart,
	ContractEnd,
	CrowdNPC_Died,
	Dart_Hit,
	DeadBodySeen,
	Disguise,
	DisguiseBlown,
	Door_Unlocked,
	Drain_Pipe_Climbed,
	ExitGate,
	FirstMissedShot,
	FirstNonHeadshot,
	HoldingIllegalWeapon,
	IntroCutEnd,
	ItemDestroyed, // broken camcorder
	ItemDropped,
	ItemPickedUp,
	ItemRemovedFromInventory,
	ItemThrown,
	Kill,
	MurderedBodySeen,
	Noticed_Pacified,
	NoticedKill,
	OpportunityEvents,
	Pacify,
	SecuritySystemRecorder,
	setpieces,
	ShotsFired,
	SituationContained,
	Spotted,
	StartingSuit,
	TargetBodySpotted,
	TargetEliminated,
	TargetEscapeFoiled, // Yuki killed in Gondola
	Trespassing,
	Unnoticed_Kill,
	Unnoticed_Pacified,
	Witnesses,
};

enum class SecuritySystemRecorderEvent {
	Undefined,
	Spotted,
	Erased,
	Destroyed,
	CameraDestroyed,
};

inline auto getSecuritySystemRecorderEventFromString(const std::string& str) {
	if (str == "spotted") return SecuritySystemRecorderEvent::Spotted;
	if (str == "erased") return SecuritySystemRecorderEvent::Erased;
	if (str == "destroyed") return SecuritySystemRecorderEvent::Destroyed;
	if (str == "CameraDestroyed") return SecuritySystemRecorderEvent::CameraDestroyed;
	return SecuritySystemRecorderEvent::Undefined;
}

inline auto getMissionTypeFromString(const std::string& str) -> std::optional<MissionType> {
	static const std::unordered_map<std::string, MissionType> map = {{
		{"arcade", MissionType::Arcade},
		{"campaign", MissionType::Campaign},
		{"elusive", MissionType::Elusive},
		{"escalation", MissionType::Escalation},
		{"evergreen", MissionType::Evergreen},
		{"featured", MissionType::Featured},
		{"mission", MissionType::Mission},
		{"orbis", MissionType::Orbis},
		{"sniper", MissionType::Sniper},
		{"tutorial", MissionType::Tutorial},
		{"usercreated", MissionType::UserCreated},
		{"vsrace", MissionType::VsRace},
	}};
	auto const it = map.find(str);
	if (it != map.end()) return it->second;
	return std::nullopt;
}

inline auto getGameTensionFromValue(int n) -> EGameTension {
	switch (static_cast<EGameTension>(n)) {
	case EGameTension::EGT_Agitated: return EGameTension::EGT_Agitated;
	case EGameTension::EGT_AlertedHigh: return EGameTension::EGT_AlertedHigh;
	case EGameTension::EGT_AlertedLow: return EGameTension::EGT_AlertedLow;
	case EGameTension::EGT_Ambient: return EGameTension::EGT_Ambient;
	case EGameTension::EGT_Arrest: return EGameTension::EGT_Arrest;
	case EGameTension::EGT_Combat: return EGameTension::EGT_Combat;
	case EGameTension::EGT_Hunting: return EGameTension::EGT_Hunting;
	case EGameTension::EGT_Searching: return EGameTension::EGT_Searching;
	}
	return EGameTension::EGT_Undefined;
}

inline auto getDeathContextFromValue(int n) -> EDeathContext {
	switch (static_cast<EDeathContext>(n)) {
	case EDeathContext::eDC_ACCIDENT: return EDeathContext::eDC_ACCIDENT;
	case EDeathContext::eDC_HIDDEN: return EDeathContext::eDC_HIDDEN;
	case EDeathContext::eDC_MURDER: return EDeathContext::eDC_MURDER;
	case EDeathContext::eDC_NOT_HERO: return EDeathContext::eDC_NOT_HERO;
	}
	return EDeathContext::eDC_UNDEFINED;
}

inline auto getDeathTypeFromValue(int n) -> EDeathType {
	switch (static_cast<EDeathType>(n)) {
	case EDeathType::eDT_BLOODY_KILL: return EDeathType::eDT_BLOODY_KILL;
	case EDeathType::eDT_KILL: return EDeathType::eDT_KILL;
	case EDeathType::eDT_PACIFY: return EDeathType::eDT_PACIFY;
	}
	return EDeathType::eDT_UNDEFINED;
}

inline auto getKillTypeFromValue(int n) -> EKillType {
	switch (static_cast<EKillType>(n)) {
	case EKillType::EKillType_ChokeOut: return EKillType::EKillType_ChokeOut;
	case EKillType::EKillType_Fiberwire: return EKillType::EKillType_Fiberwire;
	case EKillType::EKillType_ItemTakeOutBack: return EKillType::EKillType_ItemTakeOutBack;
	case EKillType::EKillType_ItemTakeOutFront: return EKillType::EKillType_ItemTakeOutFront;
	case EKillType::EKillType_KnockOut: return EKillType::EKillType_KnockOut;
	case EKillType::EKillType_PistolExecute: return EKillType::EKillType_PistolExecute;
	case EKillType::EKillType_Pull: return EKillType::EKillType_Pull;
	case EKillType::EKillType_Push: return EKillType::EKillType_Push;
	case EKillType::EKillType_SnapNeck: return EKillType::EKillType_SnapNeck;
	case EKillType::EKillType_Throw: return EKillType::EKillType_Throw;
	}
	return EKillType::EKillType_Undefined;
}

inline auto getActorTypeFromValue(int n) -> EActorType
{
	switch (static_cast<EActorType>(n))
	{
		case EActorType::eAT_Civilian: return EActorType::eAT_Civilian;
		case EActorType::eAT_Guard: return EActorType::eAT_Guard;
		case EActorType::eAT_Hitman: return EActorType::eAT_Hitman;
	}
	return EActorType::eAT_Last;
}


inline auto getBehaviourTension(ECompiledBehaviorType bt) {
	switch (bt) {
		// panicked/combat guard
		case ECompiledBehaviorType::BT_StandOffArrest:
		case ECompiledBehaviorType::BT_StandOffReposition:
		case ECompiledBehaviorType::BT_CoverFightSeasonTwo:
		case ECompiledBehaviorType::BT_HomeAttackOrigin: return 3;
		case ECompiledBehaviorType::BT_CloseCombat:
		// alerted guard
		case ECompiledBehaviorType::BT_AgitatedGuard:
		case ECompiledBehaviorType::BT_AgitatedPatrol:
		case ECompiledBehaviorType::BT_DefendVIP:
		//case ECompiledBehaviorType::BT_RadioCall:
		//case ECompiledBehaviorType::BT_AlertedStand: // sitting, stands up from distraction
		case ECompiledBehaviorType::BT_SituationAct:
		case ECompiledBehaviorType::BT_SituationApproach:
		case ECompiledBehaviorType::BT_SituationFace:
		case ECompiledBehaviorType::BT_CautiousSearchPosition:
		case ECompiledBehaviorType::BT_ProtoSearchIdle: return 2;
		// scared/panicked civilian
		case ECompiledBehaviorType::BT_Flee:
		case ECompiledBehaviorType::BT_Scared:
		case ECompiledBehaviorType::BT_SituationGetHelp:
		case ECompiledBehaviorType::BT_AgitatedBystander: return 1;
	}
	return 0;
}

inline auto getTensionValue(EGameTension tension) -> int {
	switch (tension) {
		case EGameTension::EGT_Searching: return 1;
		case EGameTension::EGT_AlertedHigh: return 3;
		case EGameTension::EGT_Hunting: return 5;
		case EGameTension::EGT_Arrest: return 8;
		case EGameTension::EGT_Combat: return 10;
	}
	return 0;
}

inline auto isTensionHigher(EGameTension oldTension, EGameTension newTension) {
	return getTensionValue(newTension) > getTensionValue(oldTension);
}

inline auto isIgnorableBehaviour(ECompiledBehaviorType bt) {
	switch (bt) {
		case ECompiledBehaviorType::BT_Act:
		case ECompiledBehaviorType::BT_Error:
		case ECompiledBehaviorType::BT_Dummy:
			return true;
	}
	return false;
}

inline auto behaviourToString(ECompiledBehaviorType bt) {
	switch (bt) {
		case ECompiledBehaviorType::BT_ConditionScope: return "BT_ConditionScope";
		case ECompiledBehaviorType::BT_Random: return "BT_Random";
		case ECompiledBehaviorType::BT_Match: return "BT_Match";
		case ECompiledBehaviorType::BT_Sequence: return "BT_Sequence";
		case ECompiledBehaviorType::BT_Dummy: return "BT_Dummy";
		case ECompiledBehaviorType::BT_Dummy2: return "BT_Dummy2";
		case ECompiledBehaviorType::BT_Error: return "BT_Error";
		case ECompiledBehaviorType::BT_Wait: return "BT_Wait";
		case ECompiledBehaviorType::BT_WaitForStanding: return "BT_WaitForStanding";
		case ECompiledBehaviorType::BT_WaitBasedOnDistanceToTarget: return "BT_WaitBasedOnDistanceToTarget";
		case ECompiledBehaviorType::BT_WaitForItemHandled: return "BT_WaitForItemHandled";
		case ECompiledBehaviorType::BT_AbandonOrder: return "BT_AbandonOrder";
		case ECompiledBehaviorType::BT_CompleteOrder: return "BT_CompleteOrder";
		case ECompiledBehaviorType::BT_PlayAct: return "BT_PlayAct";
		case ECompiledBehaviorType::BT_ConfiguredAct: return "BT_ConfiguredAct";
		case ECompiledBehaviorType::BT_PlayReaction: return "BT_PlayReaction";
		case ECompiledBehaviorType::BT_SimpleReaction: return "BT_SimpleReaction";
		case ECompiledBehaviorType::BT_SituationAct: return "BT_SituationAct";
		case ECompiledBehaviorType::BT_SituationApproach: return "BT_SituationApproach";
		case ECompiledBehaviorType::BT_SituationGetHelp: return "BT_SituationGetHelp";
		case ECompiledBehaviorType::BT_SituationFace: return "BT_SituationFace";
		case ECompiledBehaviorType::BT_SituationConversation: return "BT_SituationConversation";
		case ECompiledBehaviorType::BT_Holster: return "BT_Holster";
		case ECompiledBehaviorType::BT_SpeakWait: return "BT_SpeakWait";
		case ECompiledBehaviorType::BT_SpeakWaitWithFallbackIfAlone: return "BT_SpeakWaitWithFallbackIfAlone";
		case ECompiledBehaviorType::BT_ConfiguredSpeak: return "BT_ConfiguredSpeak";
		case ECompiledBehaviorType::BT_ConditionedConfiguredSpeak: return "BT_ConditionedConfiguredSpeak";
		case ECompiledBehaviorType::BT_ConditionedConfiguredAct: return "BT_ConditionedConfiguredAct";
		case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionAckSoundDef: return "BT_SpeakCustomOrDefaultDistractionAckSoundDef";
		case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionInvestigationSoundDef: return "BT_SpeakCustomOrDefaultDistractionInvestigationSoundDef";
		case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionStndSoundDef: return "BT_SpeakCustomOrDefaultDistractionStndSoundDef";
		case ECompiledBehaviorType::BT_Pickup: return "BT_Pickup";
		case ECompiledBehaviorType::BT_Drop: return "BT_Drop";
		case ECompiledBehaviorType::BT_PlayConversation: return "BT_PlayConversation";
		case ECompiledBehaviorType::BT_PlayAnimation: return "BT_PlayAnimation";
		case ECompiledBehaviorType::BT_MoveToLocation: return "BT_MoveToLocation";
		case ECompiledBehaviorType::BT_MoveToTargetKnownPosition: return "BT_MoveToTargetKnownPosition";
		case ECompiledBehaviorType::BT_MoveToTargetActualPosition: return "BT_MoveToTargetActualPosition";
		case ECompiledBehaviorType::BT_MoveToInteraction: return "BT_MoveToInteraction";
		case ECompiledBehaviorType::BT_MoveToNPC: return "BT_MoveToNPC";
		case ECompiledBehaviorType::BT_FollowTargetKnownPosition: return "BT_FollowTargetKnownPosition";
		case ECompiledBehaviorType::BT_FollowTargetActualPosition: return "BT_FollowTargetActualPosition";
		case ECompiledBehaviorType::BT_PickUpItem: return "BT_PickUpItem";
		case ECompiledBehaviorType::BT_GrabItem: return "BT_GrabItem";
		case ECompiledBehaviorType::BT_PutDownItem: return "BT_PutDownItem";
		case ECompiledBehaviorType::BT_Search: return "BT_Search";
		case ECompiledBehaviorType::BT_LimitedSearch: return "BT_LimitedSearch";
		case ECompiledBehaviorType::BT_MoveTo: return "BT_MoveTo";
		case ECompiledBehaviorType::BT_Reposition: return "BT_Reposition";
		case ECompiledBehaviorType::BT_SituationMoveTo: return "BT_SituationMoveTo";
		case ECompiledBehaviorType::BT_FormationMove: return "BT_FormationMove";
		case ECompiledBehaviorType::BT_SituationJumpTo: return "BT_SituationJumpTo";
		case ECompiledBehaviorType::BT_AmbientWalk: return "BT_AmbientWalk";
		case ECompiledBehaviorType::BT_AmbientStand: return "BT_AmbientStand";
		case ECompiledBehaviorType::BT_CrowdAmbientStand: return "BT_CrowdAmbientStand";
		case ECompiledBehaviorType::BT_AmbientItemUse: return "BT_AmbientItemUse";
		case ECompiledBehaviorType::BT_AmbientLook: return "BT_AmbientLook";
		case ECompiledBehaviorType::BT_Act: return "BT_Act";
		case ECompiledBehaviorType::BT_Patrol: return "BT_Patrol";
		case ECompiledBehaviorType::BT_MoveToPosition: return "BT_MoveToPosition";
		case ECompiledBehaviorType::BT_AlertedStand: return "BT_AlertedStand";
		case ECompiledBehaviorType::BT_AlertedDebug: return "BT_AlertedDebug";
		case ECompiledBehaviorType::BT_AttentionToPerson: return "BT_AttentionToPerson";
		case ECompiledBehaviorType::BT_StunnedByFlashGrenade: return "BT_StunnedByFlashGrenade";
		case ECompiledBehaviorType::BT_CuriousIdle: return "BT_CuriousIdle";
		case ECompiledBehaviorType::BT_InvestigateWeapon: return "BT_InvestigateWeapon";
		case ECompiledBehaviorType::BT_DeliverWeapon: return "BT_DeliverWeapon";
		case ECompiledBehaviorType::BT_RecoverUnconscious: return "BT_RecoverUnconscious";
		case ECompiledBehaviorType::BT_GetOutfit: return "BT_GetOutfit";
		case ECompiledBehaviorType::BT_RadioCall: return "BT_RadioCall";
		case ECompiledBehaviorType::BT_EscortOut: return "BT_EscortOut";
		case ECompiledBehaviorType::BT_StashItem: return "BT_StashItem";
		case ECompiledBehaviorType::BT_CautiousSearchPosition: return "BT_CautiousSearchPosition";
		case ECompiledBehaviorType::BT_LockdownWarning: return "BT_LockdownWarning";
		case ECompiledBehaviorType::BT_WakeUpUnconscious: return "BT_WakeUpUnconscious";
		case ECompiledBehaviorType::BT_DeadBodyInvestigate: return "BT_DeadBodyInvestigate";
		case ECompiledBehaviorType::BT_GuardDeadBody: return "BT_GuardDeadBody";
		case ECompiledBehaviorType::BT_DragDeadBody: return "BT_DragDeadBody";
		case ECompiledBehaviorType::BT_CuriousBystander: return "BT_CuriousBystander";
		case ECompiledBehaviorType::BT_DeadBodyBystander: return "BT_DeadBodyBystander";
		case ECompiledBehaviorType::BT_StandOffArrest: return "BT_StandOffArrest";
		case ECompiledBehaviorType::BT_StandOffReposition: return "BT_StandOffReposition";
		case ECompiledBehaviorType::BT_StandAndAim: return "BT_StandAndAim";
		case ECompiledBehaviorType::BT_CloseCombat: return "BT_CloseCombat";
		case ECompiledBehaviorType::BT_MoveToCloseCombat: return "BT_MoveToCloseCombat";
		case ECompiledBehaviorType::BT_MoveAwayFromCloseCombat: return "BT_MoveAwayFromCloseCombat";
		case ECompiledBehaviorType::BT_CoverFightSeasonTwo: return "BT_CoverFightSeasonTwo";
		case ECompiledBehaviorType::BT_ShootFromPosition: return "BT_ShootFromPosition";
		case ECompiledBehaviorType::BT_StandAndShoot: return "BT_StandAndShoot";
		case ECompiledBehaviorType::BT_CheckLastPosition: return "BT_CheckLastPosition";
		case ECompiledBehaviorType::BT_ProtoSearchIdle: return "BT_ProtoSearchIdle";
		case ECompiledBehaviorType::BT_ProtoApproachSearchArea: return "BT_ProtoApproachSearchArea";
		case ECompiledBehaviorType::BT_ProtoSearchPosition: return "BT_ProtoSearchPosition";
		case ECompiledBehaviorType::BT_ShootTarget: return "BT_ShootTarget";
		case ECompiledBehaviorType::BT_TriggerAlarm: return "BT_TriggerAlarm";
		case ECompiledBehaviorType::BT_MoveInCover: return "BT_MoveInCover";
		case ECompiledBehaviorType::BT_MoveToCover: return "BT_MoveToCover";
		case ECompiledBehaviorType::BT_HomeAttackOrigin: return "BT_HomeAttackOrigin";
		case ECompiledBehaviorType::BT_Shoot: return "BT_Shoot";
		case ECompiledBehaviorType::BT_Aim: return "BT_Aim";
		case ECompiledBehaviorType::BT_MoveToRandomNeighbourNode: return "BT_MoveToRandomNeighbourNode";
		case ECompiledBehaviorType::BT_MoveToRandomNeighbourNodeAiming: return "BT_MoveToRandomNeighbourNodeAiming";
		case ECompiledBehaviorType::BT_MoveToAndPlayCombatPositionAct: return "BT_MoveToAndPlayCombatPositionAct";
		case ECompiledBehaviorType::BT_MoveToAimingAndPlayCombatPositionAct: return "BT_MoveToAimingAndPlayCombatPositionAct";
		case ECompiledBehaviorType::BT_PlayJumpyReaction: return "BT_PlayJumpyReaction";
		case ECompiledBehaviorType::BT_JumpyInvestigation: return "BT_JumpyInvestigation";
		case ECompiledBehaviorType::BT_AgitatedPatrol: return "BT_AgitatedPatrol";
		case ECompiledBehaviorType::BT_AgitatedGuard: return "BT_AgitatedGuard";
		case ECompiledBehaviorType::BT_HeroEscort: return "BT_HeroEscort";
		case ECompiledBehaviorType::BT_Escort: return "BT_Escort";
		case ECompiledBehaviorType::BT_ControlledFormationMove: return "BT_ControlledFormationMove";
		case ECompiledBehaviorType::BT_EscortSearch: return "BT_EscortSearch";
		case ECompiledBehaviorType::BT_LeadEscort: return "BT_LeadEscort";
		case ECompiledBehaviorType::BT_LeadEscort2: return "BT_LeadEscort2";
		case ECompiledBehaviorType::BT_AimReaction: return "BT_AimReaction";
		case ECompiledBehaviorType::BT_FollowHitman: return "BT_FollowHitman";
		case ECompiledBehaviorType::BT_RideTheLightning: return "BT_RideTheLightning";
		case ECompiledBehaviorType::BT_Scared: return "BT_Scared";
		case ECompiledBehaviorType::BT_Flee: return "BT_Flee";
		case ECompiledBehaviorType::BT_AgitatedBystander: return "BT_AgitatedBystander";
		case ECompiledBehaviorType::BT_SentryFrisk: return "BT_SentryFrisk";
		case ECompiledBehaviorType::BT_SentryIdle: return "BT_SentryIdle";
		case ECompiledBehaviorType::BT_SentryWarning: return "BT_SentryWarning";
		case ECompiledBehaviorType::BT_SentryCheckItem: return "BT_SentryCheckItem";
		case ECompiledBehaviorType::BT_VIPScared: return "BT_VIPScared";
		case ECompiledBehaviorType::BT_VIPSafeRoomTrespasser: return "BT_VIPSafeRoomTrespasser";
		case ECompiledBehaviorType::BT_DefendVIP: return "BT_DefendVIP";
		case ECompiledBehaviorType::BT_CautiousVIP: return "BT_CautiousVIP";
		case ECompiledBehaviorType::BT_CautiousGuardVIP: return "BT_CautiousGuardVIP";
		case ECompiledBehaviorType::BT_InfectedConfused: return "BT_InfectedConfused";
		case ECompiledBehaviorType::BT_EnterInfected: return "BT_EnterInfected";
		case ECompiledBehaviorType::BT_CureInfected: return "BT_CureInfected";
		case ECompiledBehaviorType::BT_SickActInfected: return "BT_SickActInfected";
		case ECompiledBehaviorType::BT_Smart: return "BT_Smart";
		case ECompiledBehaviorType::BT_Controlled: return "BT_Controlled";
		case ECompiledBehaviorType::BT_SpeakTest: return "BT_SpeakTest";
		case ECompiledBehaviorType::BT_Conversation: return "BT_Conversation";
		case ECompiledBehaviorType::BT_RunToHelp: return "BT_RunToHelp";
		case ECompiledBehaviorType::BT_WaitForDialog: return "BT_WaitForDialog";
		case ECompiledBehaviorType::BT_WaitForConfiguredAct: return "BT_WaitForConfiguredAct";
		case ECompiledBehaviorType::BT_TestFlashbangGrenadeThrow: return "BT_TestFlashbangGrenadeThrow";
		case ECompiledBehaviorType::BT_BEHAVIORS_END: return "BT_BEHAVIORS_END";
		case ECompiledBehaviorType::BT_RenewEvent: return "BT_RenewEvent";
		case ECompiledBehaviorType::BT_ExpireEvent: return "BT_ExpireEvent";
		case ECompiledBehaviorType::BT_ExpireEvents: return "BT_ExpireEvents";
		case ECompiledBehaviorType::BT_SetEventHandled: return "BT_SetEventHandled";
		case ECompiledBehaviorType::BT_RenewSharedEvent: return "BT_RenewSharedEvent";
		case ECompiledBehaviorType::BT_ExpireSharedEvent: return "BT_ExpireSharedEvent";
		case ECompiledBehaviorType::BT_ExpireAllEvents: return "BT_ExpireAllEvents";
		case ECompiledBehaviorType::BT_CreateOrJoinSituation: return "BT_CreateOrJoinSituation";
		case ECompiledBehaviorType::BT_JoinSituation: return "BT_JoinSituation";
		case ECompiledBehaviorType::BT_ForceActorToJoinSituation: return "BT_ForceActorToJoinSituation";
		case ECompiledBehaviorType::BT_JoinSituationWithActor: return "BT_JoinSituationWithActor";
		case ECompiledBehaviorType::BT_LeaveSituation: return "BT_LeaveSituation";
		case ECompiledBehaviorType::BT_Escalate: return "BT_Escalate";
		case ECompiledBehaviorType::BT_GotoPhase: return "BT_GotoPhase";
		case ECompiledBehaviorType::BT_RenewGoal: return "BT_RenewGoal";
		case ECompiledBehaviorType::BT_ExpireGoal: return "BT_ExpireGoal";
		case ECompiledBehaviorType::BT_RenewGoalOf: return "BT_RenewGoalOf";
		case ECompiledBehaviorType::BT_ExpireGoalOf: return "BT_ExpireGoalOf";
		case ECompiledBehaviorType::BT_SetTension: return "BT_SetTension";
		case ECompiledBehaviorType::BT_TriggerSpotted: return "BT_TriggerSpotted";
		case ECompiledBehaviorType::BT_CopyKnownLocation: return "BT_CopyKnownLocation";
		case ECompiledBehaviorType::BT_UpdateKnownLocation: return "BT_UpdateKnownLocation";
		case ECompiledBehaviorType::BT_TransferKnownObjectPositions: return "BT_TransferKnownObjectPositions";
		case ECompiledBehaviorType::BT_WitnessAttack: return "BT_WitnessAttack";
		case ECompiledBehaviorType::BT_Speak: return "BT_Speak";
		case ECompiledBehaviorType::BT_StartDynamicEnforcer: return "BT_StartDynamicEnforcer";
		case ECompiledBehaviorType::BT_StopDynamicEnforcer: return "BT_StopDynamicEnforcer";
		case ECompiledBehaviorType::BT_StartRangeBasedDynamicEnforcer: return "BT_StartRangeBasedDynamicEnforcer";
		case ECompiledBehaviorType::BT_StopRangeBasedDynamicEnforcerForLocation: return "BT_StopRangeBasedDynamicEnforcerForLocation";
		case ECompiledBehaviorType::BT_StopRangeBasedDynamicEnforcer: return "BT_StopRangeBasedDynamicEnforcer";
		case ECompiledBehaviorType::BT_SetDistracted: return "BT_SetDistracted";
		case ECompiledBehaviorType::BT_IgnoreAllDistractionsExceptTheNewest: return "BT_IgnoreAllDistractionsExceptTheNewest";
		case ECompiledBehaviorType::BT_IgnoreDistractions: return "BT_IgnoreDistractions";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyWillReact: return "BT_PerceptibleEntityNotifyWillReact";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyReacted: return "BT_PerceptibleEntityNotifyReacted";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyInvestigating: return "BT_PerceptibleEntityNotifyInvestigating";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyInvestigated: return "BT_PerceptibleEntityNotifyInvestigated";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyTerminate: return "BT_PerceptibleEntityNotifyTerminate";
		case ECompiledBehaviorType::BT_LeaveDistractionAssistantRole: return "BT_LeaveDistractionAssistantRole";
		case ECompiledBehaviorType::BT_LeaveDistractionAssitingGuardRole: return "BT_LeaveDistractionAssitingGuardRole";
		case ECompiledBehaviorType::BT_RequestSuitcaseAssistanceOverRadio: return "BT_RequestSuitcaseAssistanceOverRadio";
		case ECompiledBehaviorType::BT_RequestSuitcaseAssistanceFaceToFace: return "BT_RequestSuitcaseAssistanceFaceToFace";
		case ECompiledBehaviorType::BT_ExpireArrestReasons: return "BT_ExpireArrestReasons";
		case ECompiledBehaviorType::BT_SetDialogSwitch_NPCID: return "BT_SetDialogSwitch_NPCID";
		case ECompiledBehaviorType::BT_InfectedAssignToFollowPlayer: return "BT_InfectedAssignToFollowPlayer";
		case ECompiledBehaviorType::BT_InfectedRemoveFromFollowPlayer: return "BT_InfectedRemoveFromFollowPlayer";
		case ECompiledBehaviorType::BT_Log: return "BT_Log";
		case ECompiledBehaviorType::BT_COMMANDS_END: return "BT_COMMANDS_END";
		case ECompiledBehaviorType::BT_Invalid: return "BT_Invalid";
		default: return "<unknown>";
	}
}
