#pragma once
#include "log/Logger.hpp"
#ifdef _WIN32
#include <windows.h>
#endif

namespace Extendify {
	extern log::Logger logger;
	enum class InitStatus : uint8_t {
		NOT_STARTED,
		OK,
		ERR
	};

	InitStatus runStart();
	InitStatus runStop();

	namespace ids {
		typedef const unsigned char ExtendifyId[16];
#ifdef _WIN32
		[[gnu::always_inline]] constexpr const GUID&
		extendifyIdToGUID(const ExtendifyId& eid) {
			static_assert(sizeof(GUID) == sizeof(ExtendifyId),
						  "GUID and ExtendifyId must be the same size");
			return reinterpret_cast<const GUID&>(eid);
		}

		[[gnu::always_inline]] constexpr const GUID*
		extendifyIdToGUID(ExtendifyId* eid) {
			static_assert(sizeof(GUID) == sizeof(ExtendifyId),
						  "GUID and ExtendifyId must be the same size");
			return reinterpret_cast<const GUID*>(eid);
		}
#endif
		// go to https://guidgenerator.com/ to generate
		// paste it onto a blank line
		// run this regex
		// S/[^\s]{2}/0x\0,/g

		constexpr ExtendifyId DEFAULT = {0xf6,
										 0x0c,
										 0xe4,
										 0xb6,
										 0xd7,
										 0xe2,
										 0x4a,
										 0xd2,
										 0xb5,
										 0x85,
										 0x28,
										 0xbe,
										 0xd3,
										 0xfc,
										 0x8c,
										 0xd0};

		constexpr ExtendifyId THEMES = {
			0x69,
			0x1e,
			0xe9,
			0x23,
			0x19,
			0xd6,
			0x4b,
			0xce,
			0xba,
			0x8d,
			0x22,
			0x38,
			0x16,
			0x09,
			0x24,
			0xff,
		};
	} // namespace ids
} // namespace Extendify
