#pragma once

#include <map>
#include "xdr/Stellar-ledger-entries.h"

#include <cstdint>

#include "ledger/AssetPair.h"

namespace stellar
{

class IOCOrderbookManager;
class LiquidityPoolSetFrame;
struct SupplyDemand;
class TradeMaximizingSolver;

class DemandOracle {
	using int128_t = __int128;

	const IOCOrderbookManager& mOrderbooks;
	const LiquidityPoolSetFrame& mLiquidityPools;

	int128_t demandQueryOneAssetPair(AssetPair const& tradingPair, std::map<Asset, uint64_t> const& prices) const;


public:

	DemandOracle(IOCOrderbookManager const& orderbooks, LiquidityPoolSetFrame const& liquidityPools);

	SupplyDemand demandQuery(std::map<Asset, uint64_t> const& prices, uint8_t smoothMult) const;

	void setSolverUpperBounds(TradeMaximizingSolver& solver, std::map<Asset, uint64_t> const& prices) const;
};


} /* stellar */