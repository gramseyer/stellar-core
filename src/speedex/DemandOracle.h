#pragma once

#include <map>
#include "xdr/Stellar-ledger-entries.h"

#include <cstdint>

namespace stellar
{

class IOCOrderbookManager;
class LiquidityPoolSetFrame;
class SupplyDemand;

class DemandOracle {

	const IOCOrderbookManager& mOrderbooks;
	const LiquidityPoolSetFrame& mLiquidityPools;

public:

	DemandOracle(IOCOrderbookManager const& orderbooks, LiquidityPoolSetFrame const& liquidityPools);

	SupplyDemand demandQuery(std::map<Asset, uint64_t> const& prices, uint8_t smoothMult) const;
};


} /* stellar */