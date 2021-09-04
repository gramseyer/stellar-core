#pragma once

#include "speedex/uint256_t.h"

#include "ledger/AssetPair.h"

#include <cstdint>
#include <map>

#include "util/XDROperators.h"

namespace stellar
{

class TatonnementObjectiveFn;

struct SupplyDemand {
	using int128_t = __int128;

	// pairs are (supply, demand)
	std::map<Asset, std::pair<int128_t, int128_t>> mSupplyDemand;

	void addSupplyDemand(Asset const& sell, Asset const& buy, int128_t amount);
	void addSupplyDemand(AssetPair const& tradingPair, int128_t amount);

	TatonnementObjectiveFn getObjective() const;

	int128_t getDelta(Asset const& asset) const;

};

class TatonnementObjectiveFn
{
	using int128_t = __int128;

	uint256_t value;

public:

	TatonnementObjectiveFn(std::map<Asset, std::pair<int128_t, int128_t>> const& excessDemands);

	// is self <= other * tolN/tolD?
	bool isBetterThan(TatonnementObjectiveFn const& other, uint8_t tolN, uint8_t tolD) const;
};


} /* stellar */