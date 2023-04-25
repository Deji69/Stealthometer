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

// TODO: Uhm, something better and easier to balance :/
/*
 * We're experimenting.
 *
 * So far I figure the best process for determining score values is to start with 1000 pts
 * and divide by how many times a each action should have to be done to get that, depending
 * on how likely/easy that action is.
 *
 * 1000 points will act as a baseline for minimums. Target kills should take priority if
 * there are patterns and related playstyles should aim for 1000-2000pts. So maybe fire kills
 * on targets nets 600 per kill, leading to the related playstyle netting 1200 for 2 kills.
 *
 * If there is no pattern to kills, patterns in other gameplay factors should determine the
 * final play style, so keep these high enough to near 1000, but low enough that patterned
 * kills take precedence.
 *
 * Or, like, we'll just work it out as we go...
 */
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
	PlayStyleRating("Fugitive", [](const Stats& stats) {
		if (!stats.tension.arrest) return 0;
		return stats.detection.spotted * 100;
	}),
	PlayStyleRating("Notorious", [](const Stats& stats) {
		if (!stats.current.disguiseBlown) return 0;
		return stats.detection.spotted * 100;
	}),
	PlayStyleRating("Terrorist", [](const Stats& stats) {
		if (stats.kills.nonTargets < 5) return 0;
		return stats.tension.level * 6 + stats.kills.nonTargets * 5;;
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
		return static_cast<int>(stats.itemsDisposed.size() - stats.itemsObtained.size()) * 40;
	}),
	PlayStyleRating("Cleaner", [](const Stats& stats) {
		return stats.bodies.hidden * 150 - (stats.kills.total + stats.pacifies.total) * 50
			+ stats.misc.recorderErased * 100
			+ stats.detection.witnessesKilled * 100;
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
	PlayStyleRating({"Mass Murderer", "Psychopath"}, [](const Stats& stats) {
		if (stats.kills.total < 47) return 0;
		return stats.kills.nonTargets * 50;
	}),
	PlayStyleRating({"Bad Cook", "Envenomer", "Poisoner"}, [](const Stats& stats) {
		return stats.misc.targetsMadeSick * 1000;
	}),
	PlayStyleRating("Sandman", [](const Stats& stats) {
		return stats.pacifies.unnoticed * 100;
	}),
	PlayStyleRating("Vandal", [](const Stats& stats) {
		// TODO: add setpiece/object destruction
		auto pts = stats.misc.camerasDestroyed * 100;
		pts += stats.misc.setpiecesDestroyed * 200;
		pts += stats.misc.objectsDestroyed * 150;
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
