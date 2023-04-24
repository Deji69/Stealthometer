#pragma once
#include <array>
#include "PlayStyleRating.h"
#include "Stats.h"
#include "FixMinMax.h"

enum class RatingEventType
{
	BodyHidden,
	CaughtOnCamera,
	Climb,
	DestroyCamera,
	DestroyRecorder,
	EraseRecording,
	EscapeArrest,
	HackRecorder,
	HandToHandCombatCivilian,
	HandToHandCombatGuard,
	NonTargetKilled,
	Trespass,
};

inline const auto playStyleRatings = std::array{
	PlayStyleRating("Reckless", [](const Stats& stats) {
		return stats.misc.disguisesBlown * 50
			+ stats.kills.nonTargets * 30
			+ stats.kills.noticed * 30
			+ stats.detection.spotted * 20
			+ stats.detection.onCamera * 20;
	}),
	PlayStyleRating("Bad Actor", [](const Stats& stats) {
		return stats.misc.disguisesBlown * 250;
	}),
	PlayStyleRating("Chameleon", [](const Stats& stats) {
		return stats.misc.disguisesTaken * 200;
	}),
	PlayStyleRating("Method Actor", [](const Stats& stats) {
		if (stats.misc.disguisesBlown) return 0;
		return stats.misc.disguisesTaken * 250;
	}),
	PlayStyleRating("Boxer", [](const Stats& stats) {
		return stats.misc.closeCombatEngagements * 350;
	}),
	PlayStyleRating("Criminal", [](const Stats& stats) {
		if (!stats.tension.arrest && !stats.tension.combat && !stats.tension.alertedHigh) return 0;
		return std::min(stats.detection.spotted, 3) * 250;
	}),
	PlayStyleRating("Terrorist", [](const Stats& stats) {
		return stats.tension.level * 6;
	}),
	PlayStyleRating("Hacker", [](const Stats& stats) {
		return stats.misc.recorderErased * 300;
	}),
	PlayStyleRating("Locksmith", [](const Stats& stats) {
		return stats.misc.doorsUnlocked * 50;
	}),
	PlayStyleRating("Infiltrator", [](const Stats& stats) {
		return stats.misc.doorsUnlocked * 25
			+ stats.misc.agilityActions * 25
			+ stats.misc.timesTrespassed * 25;
	}),
	PlayStyleRating("Spy", [](const Stats& stats) {
		int count = 0;
		for (auto const& item : stats.itemsObtained) {
			switch (item.second.type) {
			case ItemInfoType::Intel:
				++count;
				break;
			default: break;
			}
		}
		return count * 200;
	}),
	PlayStyleRating("Hoarder", [](const Stats& stats) {
		int count = 0;
		for (auto const& item : stats.itemsObtained) {
			switch (item.second.type) {
			case ItemInfoType::AmmoBox:
			case ItemInfoType::Briefcase:
			case ItemInfoType::Detonator:
			case ItemInfoType::Intel:
			case ItemInfoType::Key:
				break;
			default:
				++count;
				break;
			}
		}
		if (count < 5) return 0;
		return count * 50;
	}),
	PlayStyleRating("Thief", [](const Stats& stats) {
		int count = 0;
		for (auto const& item : stats.itemsObtained) {
			switch (item.second.type) {
			case ItemInfoType::Briefcase:
			case ItemInfoType::Detonator:
			case ItemInfoType::Intel:
				break;
			default:
				++count;
				break;
			}
		}
		if (!count) return 0;
		return count * 50 + stats.misc.doorsUnlocked * 25;
	}),
	PlayStyleRating("Litterer", [](const Stats& stats) {
		return static_cast<int>(stats.itemsDisposed
		.size() - stats.itemsObtained.size()) * 40;
	}),
	PlayStyleRating("Executioner", [](const Stats& stats) {
		return stats.kills.noticed * 60;
	}),
	PlayStyleRating("Murderer", [](const Stats& stats) {
		if (stats.kills.total >= 47) return 0;
		return stats.kills.nonTargets * 50;
	}),
	PlayStyleRating("Serial Killer", [](const Stats& stats) {
		if (stats.kills.total < 3 || stats.kills.noticed > 0) return 0;
		if (stats.detection.witnesses > stats.detection.witnessesKilled) return 0;
		return stats.kills.total * 60;
	}),
	PlayStyleRating({"Mass Murderer", "Psychopath", "Terrorist"}, [](const Stats& stats) {
		if (stats.kills.total < 47) return 0;
		return stats.kills.nonTargets * 50;
	}),
	PlayStyleRating({"Bad Cook", "Envenomer", "Poisoner"}, [](const Stats& stats) {
		return stats.misc.targetsMadeSick * 1000;
	}),
	PlayStyleRating("Vandal", [](const Stats& stats) {
		// TODO: add setpiece/object destruction
		auto pts = stats.misc.camerasDestroyed * 100;
		pts += stats.misc.setpiecesDestroyed * 200;
		pts += stats.misc.recorderDestroyed ? 150 : 0;
		return pts;
	}),
	PlayStyleRating({"Climber", "Hitmantler", "Spider-Hitman", "Traceur"}, [](const Stats& stats) {
		return stats.misc.agilityActions * 50;
	}),
	PlayStyleRating("Civilian", [](const Stats& stats) {
		return (
			stats.spottedBy.size()
			+ stats.misc.timesTrespassed
			+ stats.misc.camerasDestroyed
			+ stats.misc.disguisesTaken
			+ stats.misc.itemsThrown
			+ stats.misc.shotsFired
			+ stats.kills.total
			+ stats.pacifies.total
			+ stats.tension.alertedHigh
			+ stats.tension.arrest
			+ stats.tension.combat
			+ stats.tension.hunting
			+ stats.tension.searching
		) == 0 ? 10 : 0;
	}),
	PlayStyleRating("Enigma", [](const Stats& stats) {
		return 1;
	}),
};

auto getPlayStyleRating(const Stats& stats) {
	const PlayStyleRating* topRating = nullptr;
	int topScore = -1;

	for (auto const& rating : playStyleRatings) {
		auto score = rating.getScore(stats);
		if (score > topScore) {
			topRating = &rating;
			topScore = score;
		}
	}

	return topRating;
}
