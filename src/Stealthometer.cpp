#include <Windows.h>
#include <ranges>
#include <thread>
#include <Hooks.h>
#include <Logging.h>
#include <Functions.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/SOnlineEvent.h>
#include <Glacier/ZAIGameState.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZKnowledge.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZScene.h>
#include <IconsMaterialDesign.h>

#include "Stealthometer.h"
#include "json.hpp"
#include "FixMinMax.h"

HINSTANCE hInstance = nullptr;
HWND hWnd = nullptr;
ATOM wclAtom = NULL;

auto APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) -> BOOL
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			hInstance = module;
			break;
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

Stealthometer::Stealthometer() : window(this->displayStats)
{
}

Stealthometer::~Stealthometer()
{
	const ZMemberDelegate<Stealthometer, void(const SGameUpdateEvent&)> frameUpdateDelegate(this, &Stealthometer::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(frameUpdateDelegate, 1, EUpdateMode::eUpdatePlayMode);
}

auto Stealthometer::Init() -> void
{
	InitializeSRWLock(&this->eventLock);

	//Hooks::ZKnowledge_SetGameTension->AddDetour(this, &Stealthometer::ZKnowledge_SetGameTension);
	Hooks::ZAchievementManagerSimple_OnEventSent->AddDetour(this, &Stealthometer::ZAchievementManagerSimple_OnEventSent);
	this->window.create(hInstance);
}

auto Stealthometer::OnEngineInitialized() -> void
{
	const ZMemberDelegate<Stealthometer, void(const SGameUpdateEvent&)> frameUpdateDelegate(this, &Stealthometer::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(frameUpdateDelegate, 1, EUpdateMode::eUpdatePlayMode);
	this->window.create(hInstance);
}

auto isIgnorableBehaviour(ECompiledBehaviorType bt)
{
	switch (bt) {
		case ECompiledBehaviorType::BT_Act:
		case ECompiledBehaviorType::BT_Error:
		case ECompiledBehaviorType::BT_Dummy:
			return true;
	}
	return false;
}

auto getBehaviourTension(ECompiledBehaviorType bt)
{
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
		case ECompiledBehaviorType::BT_AlertedStand:
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

auto behaviourToString(ECompiledBehaviorType bt)
{
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

auto Stealthometer::IsRepoIdTargetNPC(std::string id) -> bool
{
	std::transform(id.begin(), id.end(), id.begin(),[](auto c) { return std::toupper(c); });
	for (auto const& actor : this->actorData) {
		if (!actor.isTarget) continue;
		if (id == actor.repoId) return true;
	}
	return false;
}

auto Stealthometer::OnFrameUpdate(const SGameUpdateEvent& ev) -> void
{
	for (int i = 0; i < *Globals::NextActorId; ++i) {
		const auto& actor = Globals::ActorManager->m_aActiveActors[i];
		const auto actorSpatial = actor.m_ref.QueryInterface<ZSpatialEntity>();
		auto& actorData = this->actorData[i];

		if (!actorData.ref) {
			actorData.ref = &actor;
			auto repoEntity = actor.m_ref.QueryInterface<ZRepositoryItemEntity>();
			actorData.repoId = repoEntity->m_sId.ToString();
			actorData.isTarget = actor.m_pInterfaceRef->m_bUnk16;
		}

		if (!actorSpatial)
			continue;

		if (i > this->npcCount)
			this->npcCount = i;

		if (actor.m_pInterfaceRef->m_nCurrentBehaviorIndex >= 0) {
			// (&behaviour + 0xD8) = m_pPreviousBehavior ?

			auto& behaviour = Globals::BehaviorService->m_aKnowledgeData[actor.m_pInterfaceRef->m_nCurrentBehaviorIndex];
			if (!behaviour.m_pCurrentBehavior) continue;
			auto behaviourType = static_cast<ECompiledBehaviorType>(behaviour.m_pCurrentBehavior->m_Type);
			auto lastBehaviourType = actorData.lastFrameBehaviour;
			actorData.lastFrameBehaviour = behaviourType;

			if (lastBehaviourType == behaviourType) continue;

			auto tension = getBehaviourTension(behaviourType);

			Logger::Debug("{}: {}", behaviourToString(behaviourType), tension);

			if (!tension) continue;

			if (tension > actorData.highestTensionLevel) {
				if (actorData.highestTensionLevel) tension -= actorData.highestTensionLevel;
				actorData.highestTensionLevel += tension;
				this->stats.tension.level += tension;
				this->UpdateDisplayStats();
			}
		}
	}
}

auto Stealthometer::OnDrawMenu() -> void
{
	if (ImGui::Button(ICON_MD_PIE_CHART " STEALTHOMETER"))
		this->statVisibleUI = !this->statVisibleUI;
}

auto Stealthometer::DrawSettingsUI(bool focused) -> void
{
	ImGui::PushFont(SDK()->GetImGuiBlackFont());

	ImGui::SetNextWindowSizeConstraints(ImVec2{100, 300}, ImVec2{500, 500});

	if (ImGui::Begin(ICON_MD_SETTINGS " STEALTHOMETER", &this->statVisibleUI, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::PushFont(SDK()->GetImGuiRegularFont());

		if (ImGui::Checkbox("External Window", &this->externalWindowEnabled)) {
			if (this->externalWindowEnabled) this->window.create(hInstance);
			else this->window.destroy();
		}
		if (ImGui::Checkbox("External Window Dark Mode", &this->externalWindowDarkMode))
			this->window.setDarkMode(this->externalWindowDarkMode);

		if (ImGui::Checkbox("External Window On Top", &this->externalWindowOnTop))
			this->window.setAlwaysOnTop(this->externalWindowOnTop);

		if (ImGui::Button("Kill Stats")) this->killsWindowOpen = true;
		ImGui::SameLine();
		if (ImGui::Button("KO Stats")) this->pacifiesWindowOpen = true;
		if (ImGui::Button("Misc Stats")) this->miscWindowOpen = true;

		ImGui::PopFont();
	}

	ImGui::End();
	ImGui::PopFont();
}

auto Stealthometer::DrawExpandedStatsUI(bool focused) -> void
{
	auto const& stats = this->stats;
	auto printRow = []<typename T>(const char* label, const char* fmt, T arg) {
		if (ImGui::TableNextColumn()) ImGui::Text(fmt, arg);
		if (ImGui::TableNextColumn()) ImGui::TextUnformatted(label);
	};

	ImGui::PushFont(SDK()->GetImGuiBlackFont());

	if (this->killsWindowOpen) {
		ImGui::SetNextWindowSizeConstraints(ImVec2 { 250, 200 }, ImVec2 { 600, -1 });

		if (ImGui::Begin(ICON_MD_PIE_CHART " KILLS", &this->killsWindowOpen)) {
			ImGui::PushFont(SDK()->GetImGuiRegularFont());

			if (ImGui::BeginChild("KillsL", ImVec2{ImGui::GetContentRegionAvail().x * .5f, 115})) {
				if (ImGui::BeginTable("KillStatsTableL", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
					printRow("Guard", "%d", stats.kills.guard);
					printRow("Target", "%d", stats.kills.targets);
					printRow("Unnoticed", "%d", stats.kills.unnoticed);
					printRow("Unnoticed Non-Target", "%d", stats.kills.unnoticedNonTarget);
				}
				ImGui::EndTable();
			}

			ImGui::EndChild();
			ImGui::SameLine();

			if (ImGui::BeginChild("KillsR", ImVec2{0, 110})) {
				if (ImGui::BeginTable("KillStatsTableR", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
					printRow("Civilian", "%d", stats.kills.civilian);
					printRow("Non-Target", "%d", stats.kills.nonTargets);
					printRow("Noticed", "%d", stats.kills.noticed);
				}
				ImGui::EndTable();
			}

			ImGui::EndChild();

			if (ImGui::BeginTable("KillStatsTableT", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
				printRow("Total", "%d", stats.kills.total);
			}

			ImGui::EndTable();
			ImGui::PopFont();
		}
		ImGui::End();
	}

	if (this->pacifiesWindowOpen) {
		ImGui::SetNextWindowSizeConstraints(ImVec2 { 250, 200 }, ImVec2 { 600, -1 });

		if (ImGui::Begin(ICON_MD_PIE_CHART " PACIFICATIONS", &this->pacifiesWindowOpen)) {
			ImGui::PushFont(SDK()->GetImGuiRegularFont());

			if (ImGui::BeginChild("PacificationsL", ImVec2{ImGui::GetContentRegionAvail().x * .5f, 115})) {
				if (ImGui::BeginTable("KOStatsTableL", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
					printRow("Guard", "%d", stats.pacifies.guard);
					printRow("Target", "%d", stats.pacifies.targets);
					printRow("Unnoticed", "%d", stats.pacifies.unnoticed);
					printRow("Unnoticed Non-Target", "%d", stats.pacifies.unnoticedNonTarget);
				}
				ImGui::EndTable();
			}
			ImGui::EndChild();
			ImGui::SameLine();
			if (ImGui::BeginChild("PacificationsR", ImVec2{0, 110})) {
				if (ImGui::BeginTable("KOStatsTableR", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
					printRow("Civilian", "%d", stats.pacifies.civilian);
					printRow("Non-Target", "%d", stats.pacifies.nonTargets);
					printRow("Noticed", "%d", stats.pacifies.noticed);
				}
				ImGui::EndTable();
			}
			ImGui::EndChild();
			if (ImGui::BeginTable("KOStatsTableT", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
				printRow("Total", "%d", stats.pacifies.total);
			}
			ImGui::EndTable();
			ImGui::PopFont();
		}
		ImGui::End();
	}

	if (this->miscWindowOpen) {
		ImGui::SetNextWindowSizeConstraints(ImVec2 { 250, 200 }, ImVec2 { 600, -1 });

		if (ImGui::Begin(ICON_MD_PIE_CHART " MISC STATS", &this->miscWindowOpen)) {
			ImGui::PushFont(SDK()->GetImGuiRegularFont());

			if (ImGui::BeginTable("MiscTableL", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 15);
				printRow("Agilities", "%d", stats.misc.agilityActions);
				printRow("Cameras Destroyed", "%d", stats.misc.camerasDestroyed);
				printRow("Disguises Blown", "%d", stats.misc.disguisesBlown);
				printRow("Disguises Taken", "%d", stats.misc.disguisesTaken);
				printRow("Doors Unlocked", "%d", stats.misc.doorsUnlocked);
				printRow("Items Obtained", "%d", stats.misc.itemsPickedUp);
				printRow("Items Lost", "%d", stats.misc.itemsRemovedFromInventory);
				printRow("Items Thrown", "%d", stats.misc.itemsThrown);
				printRow("Targets Made Sick", "%d", stats.misc.targetsMadeSick);
				printRow("Times Trespassed", "%d", stats.misc.timesTrespassed);
			}
			ImGui::EndTable();
			ImGui::PopFont();
		}
		ImGui::End();
	}

	ImGui::PopFont();
}

auto Stealthometer::OnDrawUI(bool focused) -> void
{
	this->DrawExpandedStatsUI(focused);

	if (!this->statVisibleUI) return;

	this->DrawSettingsUI(focused);
}

auto Stealthometer::NewContract() -> void
{
	for (auto& actorData : this->actorData) {
		actorData = ActorData{};
	}

	this->stats = Stats();
	this->displayStats = DisplayStats();
	this->npcCount = 0;
	this->eventHistory.clear();
	this->window.update();
}

auto getTensionValue(EGameTension tension) -> int
{
	switch (tension)
	{
		case EGameTension::EGT_Searching: return 1;
		case EGameTension::EGT_AlertedHigh: return 3;
		case EGameTension::EGT_Hunting: return 5;
		case EGameTension::EGT_Arrest: return 10;
		case EGameTension::EGT_Combat: return 15;
	}
	return 0;
}

auto Stealthometer::CalculateStealthRating() -> double
{
	auto rating = 100.0;
	rating -= this->stats.kills.civilian * 8;
	rating -= this->stats.kills.guard * 5;
	rating -= this->displayStats.witnesses * 10;
	rating -= this->stats.detection.onCamera * 15;
	rating -= this->displayStats.bodiesFound * 5;
	rating -= this->stats.detection.spotted * 5;
	rating -= std::max(this->stats.pacifies.nonTargets - 3, 0) * 2;
	rating += this->displayStats.bodiesHidden * 3;
	// TODO: more + adjustments
	return std::min(std::max(rating, 0.0), 100.0);
}

auto Stealthometer::UpdateDisplayStats() -> void
{
	this->window.update();

	auto updated = false;

	// Tension
	auto level = this->stats.tension.level;
	auto witness = static_cast<int>(this->stats.witnesses.size());
	auto tension = std::min(level + witness, 470);

	if (tension != this->displayStats.tension) {
		this->displayStats.tension = tension;
		updated = true;
	}

	// Pacifications
	if (this->displayStats.pacifications != this->stats.pacifies.nonTargets) {
		this->displayStats.pacifications = this->stats.pacifies.nonTargets;
		updated = true;
	}

	// Spotted
	if (this->displayStats.spotted != (this->stats.detection.targetsSpottedBy + this->stats.detection.nonTargetsSpottedBy)) {
		this->displayStats.spotted = this->stats.detection.targetsSpottedBy + this->stats.detection.nonTargetsSpottedBy;
		updated = true;
	}

	// Bodies Found
	if (this->displayStats.bodiesFound != this->stats.bodies.found) {
		this->displayStats.bodiesFound = this->stats.bodies.found;
		updated = true;
	}

	// Disguises Taken
	if (this->displayStats.disguisesTaken != this->stats.misc.disguisesTaken) {
		this->displayStats.disguisesTaken = this->stats.misc.disguisesTaken;
		updated = true;
	}

	// Recorded
	if (this->displayStats.recorded != this->stats.detection.onCamera) {
		this->displayStats.recorded = this->stats.detection.onCamera;
		updated = true;
	}

	// Guard Kills
	if (this->displayStats.guardKills != this->stats.kills.guard) {
		this->displayStats.guardKills = this->stats.kills.guard;
		updated = true;
	}

	// Civilian Kills
	if (this->displayStats.civilianKills != this->stats.kills.civilian) {
		this->displayStats.civilianKills = this->stats.kills.civilian;
		updated = true;
	}

	// Witnesses
	if (this->displayStats.witnesses != this->stats.witnesses.size()) {
		this->displayStats.witnesses = static_cast<int>(this->stats.witnesses.size());
		updated = true;
	}

	// Bodies Hidden
	if (this->displayStats.bodiesHidden != this->stats.bodies.hidden) {
		this->displayStats.bodiesHidden = this->stats.bodies.hidden;
		updated = true;
	}

	// Disguises Blown
	if (this->displayStats.disguisesBlown != this->stats.misc.disguisesBlown) {
		this->displayStats.disguisesBlown = this->stats.misc.disguisesBlown;
		updated = true;
	}

	// Targets Found
	const auto targetsFound = this->stats.bodies.targetsFound > 0;
	if (this->displayStats.targetsFound != targetsFound) {
		this->displayStats.targetsFound = targetsFound;
		updated = true;
	}

	//this->displayStats.stealthRating // TODO
	//this->displayStats.playStyle // TODO

	auto sa = SilentAssassinStatus::OK;
	if (this->stats.bodies.found || this->stats.kills.nonTargets > 0)
		sa = SilentAssassinStatus::Fail;
	else if (this->stats.detection.nonTargetsSpottedBy)
		sa = SilentAssassinStatus::Fail;
	// TODO: check the target killed was actually who spotted - right now you can kill another target and it says SA
	else if (this->stats.detection.targetsSpottedBy > this->stats.detection.targetsSpottedByAndKilled)
		sa = SilentAssassinStatus::RedeemableTarget;

	if (this->stats.detection.onCamera) {
		if (sa == SilentAssassinStatus::OK) sa = SilentAssassinStatus::RedeemableCamera;
		else if (sa == SilentAssassinStatus::RedeemableTarget) sa = SilentAssassinStatus::RedeemableCameraAndTarget;
	}

	if (this->displayStats.silentAssassin != sa) {
		this->displayStats.silentAssassin = sa;
		updated = true;
	}

	auto rating = this->CalculateStealthRating();
	if (static_cast<int>(rating * 100) != static_cast<int>(this->displayStats.stealthRating * 100)) {
		this->displayStats.stealthRating = rating;
		updated = true;
	}

	if (updated) this->window.update();
}

auto isTensionRaised(EGameTension oldTension, EGameTension newTension)
{
	return getTensionValue(newTension) > getTensionValue(oldTension);
}

//DECLARE_PLUGIN_DETOUR(Stealthometer, void, ZKnowledge_SetGameTension, ZKnowledge* knowledge, EGameTension tension)
//{
//	//Logger::Debug("Game Tension Set {:x}", knowledge);
//}

DEFINE_PLUGIN_DETOUR(Stealthometer, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventId, const ZDynamicObject& ev)
{
	ZString s_EventData;
	Functions::ZDynamicObject_ToString->Call(const_cast<ZDynamicObject*>(&ev), &s_EventData);

	auto eventDataSV = std::string_view(s_EventData.c_str(), s_EventData.size());
	auto fixedEventDataStr = std::string(s_EventData.size(), '\0');
	std::remove_copy(eventDataSV.cbegin(), eventDataSV.cend(), fixedEventDataStr.begin(), '\n');

	try {
		auto s_JsonEvent = nlohmann::json::parse(fixedEventDataStr.c_str(), fixedEventDataStr.c_str() + fixedEventDataStr.size());

		const std::string eventName = s_JsonEvent["Name"];
		Stats& stats = this->stats;

		if (eventName == "ContractStart") {
			this->NewContract();
		}
		else if (eventName == "SetupTarget") {
			Logger::Debug("SetupTarget: {}", s_EventData);
		}
		//eventName == "ItemDestroyed"
		else if (eventName == "setpieces") {
			// Blown up propane:
			// {"Timestamp":136.863831,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"2b29d641-2a0d-4781-b2dd-0df02bc2674b","name_metricvalue":"PropaneFlask","setpieceHelper_metricvalue":"PropHelper_Explosion","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"Exploded","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"4dcd73fe-7d2c-443b-aeac-10cd279ec971"}
			
			// Shot down Shisha sign:
			// {"Timestamp":6.376209,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"2d7a91b9-1b3a-4db3-a8bf-6249db70c339","name_metricvalue":"\n\n","setpieceHelper_metricvalue":"SuspendedObject","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NA","Position":"ZDynamicObject::ToString() unknown type : SVector3"},"UserId":"b1585b4d - 36f0 - 48a0 - 8ffa - 1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61 - 2714020697","Origin":"gameclient","Id":"46904050 - dd0b - 406f - a8ea - 8d56cbbca556"}
			
			// Oil drum ignited:
			// {"Timestamp":158.045746,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"032151ce-e0be-4847-aab4-1b40fcdc2bc7","name_metricvalue":"Explosive_OilDrum","setpieceHelper_metricvalue":"PropHelper_OilSpill_Flammable","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"Exploded","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"4667bd34-ed17-4eef-a46a-370904d2fe47"}
			
			// Marrakesh Toilet Drop:
			// {"Timestamp":422.665924,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"e29d8ce5-64d6-4207-a55d-ebe5e84b16b3","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"b81c3ad3-b4b6-45db-8f08-75ddeb5d919a"}
			// {"Timestamp":423.392487,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"2d7a91b9-1b3a-4db3-a8bf-6249db70c339","name_metricvalue":"ToiletDrop  ","setpieceHelper_metricvalue":"SuspendedObject","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NA","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"a533b4d4-6cfc-4f35-8f77-42e62ee9a523"}
			// {"Timestamp":423.649689,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"e29d8ce5-64d6-4207-a55d-ebe5e84b16b3","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"4990e672-515a-441a-8f9e-63c57284026c"}
			
			// Gas canister boom:
			// {"Timestamp":468.406860,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"e4bc6f9e-def7-4155-9524-16da8d68d4ad","name_metricvalue":"GasCanister_Large_A","setpieceHelper_metricvalue":"PropHelper_Explosion","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"Exploded","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"d8133825-e9ef-4d0d-b5b3-a23db08758e1"}

			// Small oil lamp (distraction object in Reza's office) destroyed:
			// {"Timestamp":466.942963,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"997fbfe6-ba8b-41a0-91bb-366bef9bef9b","name_metricvalue":"OilLamp","setpieceHelper_metricvalue":"ShotAndImpulseListener","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"OnImpact","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"d3f6d21b-6817-4cdc-b4de-6fecf8982189"}
			// {"Timestamp":466.963257,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"0f0bb2c7-1cb3-4211-87ad-555df894026a","name_metricvalue":"OilLamp","setpieceHelper_metricvalue":"DistractionLogic_Visual","setpieceType_metricvalue":"DistractionTriggered","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"f220d7c8-e0fb-48f1-b4de-6fec5dfc41f3"}
			// {"Timestamp":466.963257,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"0f0bb2c7-1cb3-4211-87ad-555df894026a","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"DistractionLogic_Visual","setpieceType_metricvalue":"DistractionFixed","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"a718105d-75f9-4d18-b4de-6fec660555f3"}

			// Chandelier winch drop
			// {"Timestamp":10.500926,"Name":"setpieces","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"683a099f-5d1b-4800-a781-5d9dfe13b12c","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"9fba84b6-85df-4d69-bd36-4376196d1202"}
			// {"Timestamp":10.607255,"Name":"setpieces","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"683a099f-5d1b-4800-a781-5d9dfe13b12c","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"af42da25-9c48-44d1-bd46-3bb88cd91c88"}
			// {"Timestamp":10.864028,"Name":"setpieces","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"2d7a91b9-1b3a-4db3-a8bf-6249db70c339","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"SuspendedObject","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NA","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"ad4e6c9d-4438-4fc1-bd6d-9697fe6ad6f0"}
			// {"Timestamp":11.056235,"Name":"Level_Setup_Events","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"Contract_Name_metricvalue":"TheShowstopper","Location_MetricValue":"Paris","Event_metricvalue":"Dahlia_Speech_Start"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"c7e69ba6-683f-427d-bd8a-cd5b0df5a43c"}
			// {"Timestamp":11.463453,"Name":"Investigate_Curious","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"ActorId":2655118168.000000,"RepositoryId":"f9c3905a-ec94-43b6-aae6-8b2f752467f7","SituationType":"AIS_INVESTIGATE_CURIOUS","EventType":"AISE_ActorJoined","JoinReason":"AISJR_Default","InvestigationType":9.000000},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"8ad219e4-5222-4f7b-bdc8-b6224db5aa6d"}
			Logger::Debug("Setpiece: {}", s_EventData);
		}
		else if (eventName == "ItemPickedUp") {
			++stats.misc.itemsPickedUp;
		}
		else if (eventName == "ItemRemovedFromInventory") {
			++stats.misc.itemsRemovedFromInventory;
		}
		else if (eventName == "ItemThrown") {
			++stats.misc.itemsThrown;
		}
		else if (eventName == "Actorsick") {
			auto isTarget = s_JsonEvent["Value"]["IsTarget"].get<bool>();
			if (isTarget) ++stats.misc.targetsMadeSick;
			Logger::Debug("ActorSick: {}", s_EventData);
		}
		else if (eventName == "Trespassing") {
			stats.current.trespassing = s_JsonEvent["Value"]["IsTrespassing"].get<bool>();
			if (stats.current.trespassing)
				++stats.misc.timesTrespassed;
		}
		else if (eventName == "SecuritySystemRecorder") {
			const auto& val = s_JsonEvent["Value"]["event"];

			if (val == "spotted")
				stats.detection.onCamera = true;
			else if (val == "destroyed" || val == "erased") {
				if (stats.detection.onCamera) stats.misc.recordedThenErased = true;
				stats.detection.onCamera = false;
				if (val == "erased") stats.misc.recorderErased = true;
				else stats.misc.recorderDestroyed = true;
			}
			else if (val == "CameraDestroyed")
				++stats.misc.camerasDestroyed;
		}
		else if (eventName == "Agility_Start" || eventName == "Drain_Pipe_Climbed") {
			++stats.misc.agilityActions;
		}
		else if (eventName == "AccidentBodyFound") {
			++stats.bodies.foundAccidents;
		}
		else if (eventName == "DeadBodySeen") {
			++stats.bodies.deadSeen;
		}
		else if (eventName == "MurderedBodySeen") {
			++stats.bodies.foundMurdered;
			if (s_JsonEvent["Value"]["DeadBody"]["IsCrowdActor"].get<bool>())
				++stats.bodies.foundCrowdMurders;
		}
		else if (eventName == "BodyFound") {
			++stats.bodies.found;
			if (s_JsonEvent["Value"]["DeadBody"]["IsCrowdActor"].get<bool>())
				++stats.bodies.foundCrowd;
		}
		else if (eventName == "Disguise") {
			++stats.misc.disguisesTaken;
			// TODO: suit retrieval?
		}
		else if (eventName == "SituationContained") {
			++stats.detection.situationsContained;
		}
		else if (eventName == "TargetBodySpotted") {
			++stats.bodies.targetsFound;
		}
		else if (eventName == "BodyHidden") {
			++stats.bodies.hidden;
		}
		else if (eventName == "BodyBagged") {
			++stats.bodies.bagged;
		}
		else if (eventName == "AllBodiesHidden") {
			stats.bodies.allHidden = true;
		}
		else if (eventName == "ShotsFired") {
			++stats.misc.shotsFired;
		}
		else if (eventName == "Spotted") {
			for (const auto& nameJson : s_JsonEvent["Value"]) {
				auto name = nameJson.get<std::string>();
				auto isTarget = this->IsRepoIdTargetNPC(name);

				Logger::Debug("Spotted by {} - Target: {}", name, isTarget);

				if (isTarget) ++stats.detection.targetsSpottedBy;

				stats.spottedBy.insert(name);
				stats.detection.nonTargetsSpottedBy = static_cast<int>(stats.spottedBy.size()) - stats.detection.targetsSpottedBy;
				++stats.detection.spotted;
			}
		}
		else if (eventName == "Witnesses") {
			for (const auto& nameJson : s_JsonEvent["Value"]) {
				stats.witnesses.insert(nameJson.get<std::string>());
				++stats.detection.witnesses;
			}
		}
		else if (eventName == "DisguiseBlown") {
			++stats.misc.disguisesBlown;
		}
		else if (eventName == "47_FoundTrespassing") {
			++stats.detection.caughtTrespassing;
		}
		else if (eventName == "TargetEliminated") {
			//++stats.kills.targets; // is this event sent in all target kill cases?
		}
		else if (eventName == "Door_Unlocked") {
			++stats.misc.doorsUnlocked;
		}
		else if (eventName == "Pacify") {
			const auto& value = s_JsonEvent["Value"];
			const auto isTarget = value["IsTarget"].get<bool>();
			const auto isAccident = value["Accident"].get<bool>();
			const auto actorType = value["ActorType"].get<EActorType>();
			const auto killClass = value["KillClass"].get<std::string>();
			const auto killMethodBroad = value["KillMethodBroad"].get<std::string>();

			stats.bodies.allHidden = false;
			++stats.pacifies.total;

			if (isTarget) stats.bodies.allTargetsHidden = false;
			else ++stats.pacifies.nonTargets;

			if (isAccident) ++stats.pacifyMethods.accident;
			if (killClass == "melee") ++stats.pacifyMethods.melee;
			if (killMethodBroad == "throw") ++stats.pacifyMethods.thrown;

			if (!isTarget) {
				if (actorType == EActorType::eAT_Civilian) ++stats.pacifies.civilian;
				if (actorType == EActorType::eAT_Guard) ++stats.pacifies.guard;
			}
		}
		else if (eventName == "CrowdNPC_Died") {
			++stats.kills.total;
			++stats.kills.nonTargets;
			++stats.kills.civilian;
		}
		else if (eventName == "Kill") {
			const auto& value = s_JsonEvent["Value"];
			const auto repoId = value["RepositoryId"].get<std::string>();
			const auto isTarget = value["IsTarget"].get<bool>();
			const auto isAccident = value["Accident"].get<bool>();
			const auto actorType = value["ActorType"].get<EActorType>();
			const auto killClass = value["KillClass"].get<std::string>();
			const auto killMethodBroad = value["KillMethodBroad"].get<std::string>();
			const auto isWeaponSilenced = value["WeaponSilenced"].get<bool>();

			const auto killItemCategoryIt = value.find("KillItemCategory");
			const auto killItemCategory = killItemCategoryIt != value.end() ? killItemCategoryIt->get<std::string>() : "";

			stats.bodies.allHidden = false;
			++stats.kills.total;

			if (isTarget) {
				++stats.kills.targets;
				if (stats.targetsSpottedBy.count(repoId))
					++stats.detection.targetsSpottedByAndKilled;
			}
			else {
				++stats.kills.nonTargets;
				if (actorType == EActorType::eAT_Civilian) ++stats.kills.civilian;
				if (actorType == EActorType::eAT_Guard) ++stats.kills.guard;
			}

			if (value["IsHeadshot"].get<bool>()) ++stats.killMethods.headshot;
			if (killClass == "melee") ++stats.killMethods.melee;
			if (killMethodBroad == "throw") ++stats.killMethods.thrown;
			if (isWeaponSilenced) ++stats.killMethods.silencedWeapon;

			if (isAccident) ++stats.killMethods.accident;

			if (killItemCategory == "pistol")
				++stats.killMethods.pistol;

			if (stats.spottedBy.count(repoId))
				++stats.detection.uniqueNPCsCaughtByAndKilled;

			auto witnessIt = stats.witnesses.find(repoId);
			if (witnessIt != stats.witnesses.end()) {
				stats.witnesses.erase(witnessIt);
				++stats.detection.witnessesKilled;
			}
		}
		else if (eventName == "NoticedKill") {
			++stats.kills.noticed;
		}
		else if (eventName == "Noticed_Pacified") {
			++stats.pacifies.noticed;
		}
		else if (eventName == "Unnoticed_Kill") {
			const auto& val = s_JsonEvent["Value"];
			++stats.kills.unnoticed;
			if (!val["IsTarget"].get<bool>())
				++stats.kills.unnoticedNonTarget;
		}
		else if (eventName == "Unnoticed_Pacified") {
			++stats.pacifies.unnoticed;
			if (!s_JsonEvent["Value"]["IsTarget"].get<bool>())
				++stats.pacifies.unnoticedNonTarget;
		}
		else if (eventName == "AmbientChanged") {
			const auto tension = s_JsonEvent["Value"]["AmbientValue"].get<EGameTension>();
			const auto prevTension = s_JsonEvent["Value"]["PreviousAmbientValue"].get<EGameTension>();

			stats.current.tension = tension;

			switch (tension) {
			case EGameTension::EGT_Agitated:
				Logger::Debug("Game tension: agitated - it actually happened!");
				++stats.tension.agitated;
				break;
			case EGameTension::EGT_AlertedHigh:
				++stats.tension.alertedHigh;
				break;
			case EGameTension::EGT_AlertedLow:
				++stats.tension.alertedLow;
				break;
			case EGameTension::EGT_Arrest:
				++stats.tension.arrest;
				break;
			case EGameTension::EGT_Combat:
				++stats.tension.combat;
				break;
			case EGameTension::EGT_Hunting:
				++stats.tension.hunting;
				break;
			case EGameTension::EGT_Searching:
				++stats.tension.searching;
				break;
			}

			if (isTensionRaised(prevTension, tension)) {
				stats.tension.level += getTensionValue(tension) - getTensionValue(prevTension);
			}
		}
		else Logger::Debug("Unhandled Event Sent: {} - {}", eventId, s_EventData);

		this->UpdateDisplayStats();
		this->eventHistory.push_back(eventName);
	}
	catch (const nlohmann::json::exception& ex) {
		Logger::Error("JSON exception: {}", ex.what());
	}

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(Stealthometer);
