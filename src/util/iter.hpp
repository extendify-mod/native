#pragma once

#include <utility>
#include <vector>


namespace Extendify::util::iter {
	template<typename Orig, typename F>

	[[nodiscard]] auto map(const std::vector<Orig>& vec, F mapper) {
		using New = decltype(mapper(std::declval<Orig>()));
		std::vector<New> ret;
		ret.reserve(vec.size());
		for (const auto& item : vec) {
			ret.push_back(mapper(item));
		}
		return ret;
	}
} // namespace Extendify::util::iter
