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
			if (sellAsset < buyAsset) {


				AssetPair pair1 {
					.selling = sellAsset,
					.buying = buyAsset
				};
				AssetPair pair2 {
					.selling = buyAsset,
					.buying = sellAsset
				};

				mBaseFrames.emplace_back(ltx, pair1);

				mLiquidityPools.emplace(std::piecewise_construct,
					std::forward_as_tuple(pair1),
					std::forward_as_tuple(mBaseFrames.back(), pair1));

				mLiquidityPools.emplace(std::piecewise_construct,
					std::forward_as_tuple(pair2),
					std::forward_as_tuple(mBaseFrames.back(), pair2));
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

LiquidityPoolSetFrame::int128_t
LiquidityPoolSetFrame::demandQueryOneAssetPair(AssetPair const& tradingPair, std::map<Asset, uint64_t> const& prices) const
{
	auto iter = mLiquidityPools.find(tradingPair);
	if (iter == mLiquidityPools.end()) {
		return 0;
	}
	return iter->second.amountOfferedForSaleTimesSellPrice(prices.at(tradingPair.selling), prices.at(tradingPair.buying));
}


} /* stellar */