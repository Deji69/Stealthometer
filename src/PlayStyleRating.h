#pragma once
#include <functional>
#include <string>
#include <vector>

class Stats;

class PlayStyleRating
{
public:
	PlayStyleRating(std::vector<std::string> titles, std::function<int(const Stats&)> func) : titles(titles), calculateScore(func)
	{}
	PlayStyleRating(const char* title, std::function<int(const Stats&)> func) : PlayStyleRating(std::vector { std::string(title) }, func)
	{}

	auto getScore(const Stats& stats) const { return calculateScore(stats); }
	auto& getTitle(size_t n) const { return titles[n]; }
	auto& getTitles() const { return titles; }

private:
	std::vector<std::string> titles;
	std::function<int(const Stats&)> calculateScore;
};
