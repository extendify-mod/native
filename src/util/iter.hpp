#pragma once

#include "util.hpp"

#include <utility>
#include <vector>


namespace Extendify::util::iter {
	template<typename Orig, typename F, typename New = util::ReturnType<F, Orig>::type>

	[[nodiscard]] std::vector<New> map(const std::vector<Orig>& vec, F mapper) {
		std::vector<New> ret;
		ret.reserve(vec.size());
		for (const auto& item : vec) {
			ret.push_back(mapper(item));
		}
		return ret;
	}
} // namespace Extendify::util::iter
