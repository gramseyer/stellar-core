#include "speedex/BatchSolution.h"

namespace stellar {


BatchSolution::BatchSolution() {
	//TODO init
}


std::vector<OrderbookClearingTarget>
BatchSolution::produceClearingTargets() const {

	std::vector<OrderbookClearingTargets> out;

	for (auto& [tradingPair, amount] : mTradingAmountsTimesPrices) {
		uint64_t sellPrice = mAssetPrices[tradingPair.selling];
		uitn64_t buyPrice = mAssetPrices[tradingPair.buying];

		out.emplace_back(tradingPair, sellPrice, buyPrice, amount);
	}
	return out;
}

} /* stellar */