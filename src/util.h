#pragma once
#include <algorithm>

struct InsensitiveCompare
{
	auto operator()(const std::string& a, const std::string& b) const {
		return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](int a, int b){ return std::tolower(a) == std::tolower(b); });
	}
};

struct StringHashLowercase
{
public:
	auto operator()(const std::string& str) const {
		std::string lower;
		std::transform(str.begin(), str.end(), lower.begin(), [](int c) { return std::tolower(c); });
		return std::hash<std::string>()(lower);
	}
};
