#pragma once
#include <array>
#include "PlayStyleRating.h"
#include "Stats.h"

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
	PlayStyleRating("Bad Actor", [](const Stats& stats) {
		return stats.misc.disguisesBlown * 500;
	}),
	PlayStyleRating("Chameleon", [](const Stats& stats) {
		return stats.misc.disguisesTaken * 250;
	}),
	PlayStyleRating("Method Actor", [](const Stats& stats) {
		return stats.misc.disguisesTaken * 500 - stats.misc.disguisesBlown * 500;
	}),
	PlayStyleRating("Hacker", [](const Stats& stats) {
		return stats.misc.recorderErased * 500;
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
			+ stats.misc.agilityActions
			+ stats.misc.camerasDestroyed
			+ stats.misc.disguisesTaken
			+ stats.misc.itemsThrown
			+ stats.misc.recorderDestroyed
			+ stats.misc.recorderErased
			+ stats.misc.shotsFired
			+ stats.misc.targetsMadeSick
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
