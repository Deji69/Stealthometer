#pragma once

enum class FreelancerCampaignStage {
	FirstSyndicate,
	SecondSyndicate,
	ThirdSyndicate,
	ForthSyndicate,
};

struct FreelancerRunData
{
	FreelancerCampaignStage stage;
	bool inShowdown = false;
	bool noSyndicateActive = false;
	bool campaignInProgress = false;
	bool campaignCompleted = false;
};

struct RunData
{
	MissionType missionType;
	FreelancerRunData freelancer;
	bool shouldAutoStartLiveSplit = false;
};
