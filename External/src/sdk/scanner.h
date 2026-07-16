#pragma once
#include <cstdint>
#include <string>

namespace Scanner
{
	struct AnchorResult
	{
		std::uint64_t dataModel = 0;
		std::uint64_t visualEngine = 0;
		std::uint64_t fakeDataModel = 0;
		std::uint64_t fakeDataModelRva = 0;
		std::uint64_t visualEngineRva = 0;
		std::uint64_t realDataModelOffset = 0;
		std::string method;
		bool success = false;
	};

	// Tries hardcoded offsets first, then scans the Roblox module for valid anchors.
	AnchorResult ResolveAnchors();
}
