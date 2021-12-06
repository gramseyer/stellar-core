#include "speedex/LiquidityPoolSetFrame.h"

#include "ledger/LedgerTxn.h"

#include "speedex/DemandUtils.h"
#include "speedex/sim_utils.h"

#include <utility>

#include "util/types.h"

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

				mBaseFrames.emplace_back(std::make_unique<LiquidityPoolFrameBaseLtx>(ltx, pair1));

				mLiquidityPools.emplace(std::piecewise_construct,
					std::forward_as_tuple(pair1),
					std::forward_as_tuple(*mBaseFrames.back(), pair1));

				mLiquidityPools.emplace(std::piecewise_construct,
					std::forward_as_tuple(pair2),
					std::forward_as_tuple(*mBaseFrames.back(), pair2));
			}
		}
	}
}

LiquidityPoolSetFrame::LiquidityPoolSetFrame(SpeedexSimConfig const& sim)
{
	for (auto const& ammconfig : sim.ammConfigs)
	{
		AssetPair p = AssetPair{
			.selling = makeSimAsset(ammconfig.assetA),
			.buying = makeSimAsset(ammconfig.assetB)
		};
		if (p.buying < p.selling) {
			p = p.reverse();
		}

		mBaseFrames.emplace_back(std::make_unique<LiquidityPoolFrameBaseSim>(ammconfig));

		mLiquidityPools.emplace(std::piecewise_construct,
			std::forward_as_tuple(p),
			std::forward_as_tuple(*mBaseFrames.back(), p));
		p = p.reverse();
		mLiquidityPools.emplace(std::piecewise_construct,
			std::forward_as_tuple(p),
			std::forward_as_tuple(*mBaseFrames.back(), p));
	}
}

void
LiquidityPoolSetFrame::demandQuery(std::map<Asset, uint64_t> const& prices, SupplyDemand& supplyDemand) const
{
	for (auto const& [tradingPair, lpFrame] : mLiquidityPools)
	{
		int128_t sellAmountTimesPrice = lpFrame.amountOfferedForSaleTimesSellPrice(prices.at(tradingPair.selling), prices.at(tradingPair.buying));

		//std::printf("lp sell %s buy %s: %lf %ld\n", 
		//	assetToString(tradingPair.selling).c_str(),
		//	assetToString(tradingPair.buying).c_str(),
		//	(double) sellAmountTimesPrice,
		//	(int64_t) sellAmountTimesPrice);

		supplyDemand.addSupplyDemand(tradingPair, sellAmountTimesPrice);
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


LiquidityPoolFrame&
LiquidityPoolSetFrame::getFrame(AssetPair const& tradingPair) {
	return mLiquidityPools.at(tradingPair);
}


} /* stellar */