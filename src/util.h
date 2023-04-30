#pragma once
#include <algorithm>
#include <string>
#include <string_view>

struct InsensitiveCompare
{
	auto operator()(std::string_view a, std::string_view b) const -> bool {
		return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](unsigned char a, unsigned char b) {
			return std::tolower(a) == std::tolower(b);
		});
	}
};

struct InsensitiveCompareLexicographic
{
	auto operator()(std::string_view a, std::string_view b) const -> bool {
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](unsigned char a, unsigned char b) {
			return std::tolower(a) < std::tolower(b);
		});
	}
};

struct StringHashLowercase
{
public:
	auto operator()(const std::string& str) const {
		std::string lower;
		std::transform(str.begin(), str.end(), std::back_inserter(lower), [](unsigned char c) -> unsigned char { return std::tolower(c); });
		return std::hash<std::string>()(lower);
	}
};
