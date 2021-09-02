#pragma once

#include "ledger/AssetPair.h"

#include "speedex/IOCOrderbook.h"
#include "speedex/BatchSolution.h"

#include "util/UnorderedMap.h"
#include <map>

namespace stellar {

class AbstractLedgerTxn;
class BatchClearingTarget;
class OrderbookClearingTarget;

class IOCOrderbookManager {
	using int128_t = __int128_t;

	UnorderedMap<AssetPair, IOCOrderbook, AssetPairHash> mOrderbooks;

	bool mSealed;

	void clearOrderbook(AbstractLedgerTxn& ltx, OrderbookClearingTarget& target);

	void throwIfSealed() const;
	void throwIfNotSealed() const;

	IOCOrderbook&
	getOrCreateOrderbook(AssetPair const& assetPair);
 
	void returnToSource(AbstractLedgerTxn& ltx, Asset asset, int64_t amount);

	void doPriceComputationPreprocessing();

public:

	IOCOrderbookManager() : mSealed(false) {}

	void addOffer(AssetPair const& assetPair, IOCOffer const& offer);

	void commitChild(const IOCOrderbookManager& child);

	void clear();

	void sealBatch();

	void clearBatch(AbstractLedgerTxn& ltx, const BatchSolution& batchSolution);

	size_t numOpenOrderbooks() const;

	void demandQuery(
		std::map<Asset, uint64_t> const& prices, 
		std::map<Asset, int128_t>& demandsOut, 
		uint8_t taxRate,
		uint8_t smoothMult) const;


};

}