#pragma once

#include "ledger/AssetPair.h"
#include "speedex/LiquidityPoolFrame.h"

#include "util/UnorderedMap.h"
#include <map>

namespace stellar
{

class AbstractLedgerTxn;
struct SupplyDemand;

class LiquidityPoolSetFrame {
	UnorderedMap<AssetPair, LiquidityPoolFrame, AssetPairHash> mLiquidityPools;

	std::vector<BaseLiquidityPoolFrame> mBaseFrames;

	using int128_t = __int128;

public:

	LiquidityPoolSetFrame(std::vector<Asset> const& assets, AbstractLedgerTxn& ltx);

	void demandQuery(std::map<Asset, uint64_t> const& prices, SupplyDemand& supplyDemand) const;

	int128_t 
	demandQueryOneAssetPair(AssetPair const& tradingPair, std::map<Asset, uint64_t> const& prices) const;

	LiquidityPoolFrame&
	getFrame(AssetPair const& tradingPair);

};

} /* stellar */