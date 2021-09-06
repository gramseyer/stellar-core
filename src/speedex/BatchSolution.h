#pragma once

#include <cstdint>
#include "ledger/AssetPair.h"
#include "ledger/LedgerHashUtils.h"
#include "util/XDROperators.h"
#include "util/UnorderedMap.h"
#include <map>

namespace stellar {

class OrderbookClearingTarget;

class BatchSolution {
	using int128_t = __int128_t;

	UnorderedMap<AssetPair, int128_t, AssetPairHash> mTradeAmountsTimesPrices;

	std::map<Asset, uint64_t> mAssetPrices;

	//Note different units than in IOCOffer precomputedTatonnementStats
	//This is just (trade amount -- int64_t) * valuation (uint64_t)
public:

	BatchSolution(
		UnorderedMap<AssetPair, int128_t, AssetPairHash> const& tradeAmounts,
		std::map<Asset, uint64_t> const& prices);

	std::vector<OrderbookClearingTarget>
	produceClearingTargets() const;

};

} /* namespace stellar */