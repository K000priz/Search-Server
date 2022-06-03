#pragma once
#include<string>
#include<set>
#include<vector>
#include<string_view>

std::vector<std::string_view> SplitIntoWords(std::string_view str);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings{};

    for (const auto& sv : strings) { 
        if (!sv.empty()) {
            non_empty_strings.insert(std::string{ sv });
        }
    }
    return non_empty_strings;
}
