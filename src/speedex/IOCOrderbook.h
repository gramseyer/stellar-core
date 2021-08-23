#pragma once

#include "ledger/AssetPair.h"

#include "speedex/IOCOffer.h"

#include <set>
#include <vector>

#include "speedex/OrderbookClearingTarget.h"
#include "speedex/LiquidityPoolFrame.h"

namespace stellar {

struct IOCOrderbookClearingParams {
	Price clearingRatio;
	int64_t totalSellAmount;
};

class AbstractLedgerTxn;

class IOCOrderbook {

	using int128_t = __int128_t;

	struct PriceCompStats {
		Price marginalPrice;
		int64_t cumulativeOfferedForSale;

		int128_t cumulativeOfferedForSaleTimesPrice; // Fractional : Radix 32 bits;

		constexpr static int OFFERED_TIMES_PRICE_RADIX = 32;
	};

	const AssetPair mTradingPair;
	std::set<IOCOffer> mOffers; // sorted by IOCOffer::operator<=>

	std::vector<PriceCompStats> mPrecomputedTatonnementData;

	bool mCleared;

	void throwIfCleared();
	void throwIfNotCleared();

public:
	IOCOrderbook(AssetPair tradingPair);

	void doPriceComputationPreprocessing();

	void addOffer(IOCOffer offer);

	void commitChild(const IOCOrderbook& child);

	void clearOffers(AbstractLedgerTxn& ltx, OrderbookClearingTarget& target, LiquidityPoolFrame& lpFrame);

	void finish(AbstractLedgerTxn& ltx);
};



} /* stellar */