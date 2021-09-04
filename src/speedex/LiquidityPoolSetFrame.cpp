#include "speedex/LiquidityPoolSetFrame.h"

#include "ledger/LedgerTxn.h"

#include "speedex/DemandUtils.h"

#include <utility>

namespace stellar
{

LiquidityPoolSetFrame::LiquidityPoolSetFrame(std::vector<Asset> const& assets, AbstractLedgerTxn& ltx)
{
	for (auto const& sellAsset : assets) {
		for (auto const& buyAsset : assets) {
			if (sellAsset != buyAsset) {
				AssetPair pair {
					.selling = sellAsset,
					.buying = buyAsset
				};
				mLiquidityPools.emplace(std::piecewise_construct,
					std::forward_as_tuple(pair),
					std::forward_as_tuple(ltx, pair));
			}
		}
	}
}

void
LiquidityPoolSetFrame::demandQuery(std::map<Asset, uint64_t> const& prices, SupplyDemand& supplyDemand) const
{
	for (auto const& [tradingPair, lpFrame] : mLiquidityPools)
	{
		auto sellAmountTimesPrice = lpFrame.amountOfferedForSaleTimesSellPrice(prices.at(tradingPair.selling), prices.at(tradingPair.buying));

		supplyDemand.addSupplyDemand(tradingPair, sellAmountTimesPrice);

//		auto tax = taxRate == 0 ? 0 : sellAmountTimesPrice >> taxRate;

		//demands[tradingPair.selling] -= sellAmountTimesPrice;
		//demands[tradingPair.buying] += (sellAmountTimesPrice - tax);
	}
}


} /* stellar */