#pragma once

#include "ledger/AssetPair.h"

#include "speedex/IOCOrderbook.h"
#include "speedex/BatchSolution.h"

#include "util/UnorderedMap.h"
#include <map>

#include "xdr/Stellar-ledger.h"

namespace stellar {

class AbstractLedgerTxn;
class BatchClearingTarget;
class OrderbookClearingTarget;
struct SupplyDemand;
class LiquidityPoolSetFrame;
class LiquidityPoolFrame;

class IOCOrderbookManager {
	using int128_t = __int128_t;

	UnorderedMap<AssetPair, IOCOrderbook, AssetPairHash> mOrderbooks;

	bool mSealed;
	bool mCleared;

	std::pair<std::vector<SpeedexOfferClearingStatus>, std::optional<SpeedexLiquidityPoolClearingStatus>>
 	clearOrderbook(AbstractLedgerTxn& ltx, OrderbookClearingTarget& target, LiquidityPoolFrame& lpFrame);

	void throwIfSealed() const;
	void throwIfNotSealed() const;

	void throwIfAlreadyCleared() const;

	IOCOrderbook&
	getOrCreateOrderbook(AssetPair const& assetPair);
 
	void returnToSource(AbstractLedgerTxn& ltx, Asset asset, int64_t amount);

	void doPriceComputationPreprocessing();

public:

	IOCOrderbookManager() : mSealed(false), mCleared(false) {}

	void addOffer(AssetPair const& assetPair, IOCOffer const& offer);

	void commitChild(const IOCOrderbookManager& child);

	void clear();

	void sealBatch();

	SpeedexResults
	clearBatch(AbstractLedgerTxn& ltx, const BatchSolution& batchSolution, LiquidityPoolSetFrame& liquidityPools);

	size_t numOpenOrderbooks() const;

	void demandQuery(
		std::map<Asset, uint64_t> const& prices, 
		SupplyDemand& supplyDemand,
		uint8_t smoothMult) const;

	int128_t 
	demandQueryOneAssetPair(
		AssetPair const& tradingPair, 
		std::map<Asset, uint64_t> const& prices) const; //smooth mult = 0

	SpeedexResults
	clearSimBatch(const BatchSolution& batchSolution, LiquidityPoolSetFrame& liquidityPools);
};

}