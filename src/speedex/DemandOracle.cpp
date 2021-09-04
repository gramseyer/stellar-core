#include "speedex/DemandOracle.h"

#include "speedex/IOCOrderbookManager.h"
#include "speedex/LiquidityPoolSetFrame.h"

#include "speedex/DemandUtils.h"

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

} /* stellar */
