#include "speedex/BatchSolution.h"

#include "speedex/OrderbookClearingTarget.h"

namespace stellar {


BatchSolution::BatchSolution(UnorderedMap<AssetPair, int128_t, AssetPairHash> const& tradeAmounts, std::map<Asset, uint64_t> const& prices)
	: mTradeAmountsTimesPrices(tradeAmounts)
	, mAssetPrices(prices)
{}


std::vector<OrderbookClearingTarget>
BatchSolution::produceClearingTargets() const {

	std::vector<OrderbookClearingTarget> out;

	for (auto& [tradingPair, amount] : mTradeAmountsTimesPrices) {
		uint64_t sellPrice = mAssetPrices.at(tradingPair.selling);
		uint64_t buyPrice = mAssetPrices.at(tradingPair.buying);

		out.emplace_back(tradingPair, sellPrice, buyPrice, amount);
	}
	return out;
}

} /* stellar */