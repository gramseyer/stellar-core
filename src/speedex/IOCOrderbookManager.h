#pragma once

#include "ledger/AssetPair.h"

#include "speedex/IOCOrderbook.h"
#include "speedex/BatchSolution.h"

#include "util/UnorderedMap.h"

namespace stellar {

class AbstractLedgerTxn;
class BatchClearingTarget;
class OrderbookClearingTarget;

class IOCOrderbookManager {
	using int128_t = __int128_t;

	UnorderedMap<AssetPair, IOCOrderbook, AssetPairHash> mOrderbooks;

	bool mSealed;
	bool mTatonnementStatsPrecomputed;

	void clearOrderbook(AbstractLedgerTxn& ltx, OrderbookClearingTarget& target);

	void throwIfSealed() const;
	void throwIfNotSealed() const;

	IOCOrderbook&
	getOrCreateOrderbook(AssetPair assetPair);
 
	void returnToSource(AbstractLedgerTxn& ltx, Asset asset, int64_t amount);

	void doPriceComputationPreprocessing();

public:

	IOCOrderbookManager() : mSealed(false), mTatonnementStatsPrecomputed(false) {}

	void addOffer(AssetPair assetPair, const IOCOffer& offer);

	void commitChild(const IOCOrderbookManager& child);

	void clear();

	void sealBatch();

	void clearBatch(AbstractLedgerTxn& ltx, const BatchSolution& batchSolution);

	size_t numOpenOrderbooks() const;

	void demandQuery(
		UnorderedMap<Asset, uint64_t> const& prices, 
		UnorderedMap<Asset, int128_t>& demandsOut, 
		uint8_t taxRate,
		uint8_t smoothMult) const;


};

}