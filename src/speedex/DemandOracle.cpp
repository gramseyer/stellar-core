#include "speedex/DemandOracle.h"

#include "speedex/IOCOrderbookManager.h"
#include "speedex/LiquidityPoolSetFrame.h"

#include "speedex/DemandUtils.h"

#include "simplex/solver.h"

namespace stellar
{

DemandOracle::DemandOracle(IOCOrderbookManager const& orderbooks, LiquidityPoolSetFrame const& liquidityPools)
	: mOrderbooks(orderbooks)
	, mLiquidityPools(liquidityPools)
	{}

SupplyDemand
DemandOracle::demandQuery(std::map<Asset, uint64_t> const& prices, uint8_t smoothMult) const
{
	SupplyDemand sd;
	mOrderbooks.demandQuery(prices, sd, smoothMult);
	mLiquidityPools.demandQuery(prices, sd);
	return sd;
}

DemandOracle::int128_t
DemandOracle::demandQueryOneAssetPair(AssetPair const& tradingPair, std::map<Asset, uint64_t> const& prices) const
{
	return mOrderbooks.demandQueryOneAssetPair(tradingPair, prices)
		 + mLiquidityPools.demandQueryOneAssetPair(tradingPair, prices);
}

void
DemandOracle::setSolverUpperBounds(TradeMaximizingSolver& solver, std::map<Asset, uint64_t> const& prices) const
{
	for (auto const& [sellAsset, sellPrice] : prices)
	{
		for (auto const& [buyAsset, buyPrice] : prices)
		{
			if (buyAsset != sellAsset)
			{
				AssetPair tradingPair {
					.selling = sellAsset,
					.buying = buyAsset
				};
				int128_t supply = demandQueryOneAssetPair(tradingPair, prices);
				if (supply != 0)
				{
					solver.setUpperBound(tradingPair, supply);
				}
			}
		}
	}
}

} /* stellar */
