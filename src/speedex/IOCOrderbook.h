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

constexpr static Price zeroPrice() {
	Price p;
	p.n = 0;
	p.d = 1;
	return p;
}

class AbstractLedgerTxn;

class IOCOrderbook {

public:

	using int128_t = __int128_t;

	struct PriceCompStats {
		Price marginalPrice;
		int64_t cumulativeOfferedForSale;

		int128_t cumulativeOfferedForSaleTimesPrice; // Fractional : Radix 32 bits;

		constexpr static int OFFERED_TIMES_PRICE_RADIX = 32;

		constexpr static int128_t OFFERED_TIMES_PRICE_LOWBITS_MASK
	 		= 0xFFFFFFFF;

	};

private:

	constexpr static PriceCompStats zeroStats = PriceCompStats
	{
		.marginalPrice = zeroPrice(), 
		.cumulativeOfferedForSale = 0,
		.cumulativeOfferedForSaleTimesPrice = 0
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

	//visible for testing
	PriceCompStats getPriceCompStats(uint64_t sellPrice, uint64_t buyPrice) const;

	void addOffer(IOCOffer offer);

	void commitChild(const IOCOrderbook& child);

	void clearOffers(AbstractLedgerTxn& ltx, OrderbookClearingTarget& target, LiquidityPoolFrame& lpFrame);

	void finish(AbstractLedgerTxn& ltx);

	// output: radix 32 bits
	int128_t cumulativeOfferedForSaleTimesPrice(uint64_t sellPrice, uint64_t buyPrice, uint8_t smoothMult) const;
};



} /* stellar */