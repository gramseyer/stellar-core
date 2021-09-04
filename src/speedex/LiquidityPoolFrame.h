#pragma once

#include "ledger/LedgerTxnEntry.h"

namespace stellar {

class AbstractLedgerTxn;
struct AssetPair;

class LiquidityPoolFrame {

	using int128_t = __int128_t;

	LedgerTxnEntry mEntry;

	AssetPair const& mTradingPair;

	double getFeeRate() const;

	double getMinPriceRatio() const;

	uint32_t
	getFixedPointFeeRate() const;
	uint64_t
	subtractFeeRateFixedPoint(uint64_t startingPrice) const;

public:

	std::pair<int64_t, int64_t> 
	getSellBuyAmounts() const;

	std::pair<uint64_t, uint64_t>
	getMinPriceRatioFixedPoint() const;

	LiquidityPoolFrame(AbstractLedgerTxn& ltx, AssetPair const& assetPair);
	
	operator bool() const;

	int64_t amountOfferedForSale(uint64_t sellPrice, uint64_t buyPrice) const;

	void assertValidSellAmount(int64_t sellAmount, uint64_t sellPrice, uint64_t buyPrice) const;

	void assertValidTrade(int64_t sellAmount, int64_t buyAmount, uint64_t sellPrice, uint64_t buyPrice) const;

	void doTransfer(int64_t sellAmount, int64_t buyAmount, uint64_t sellPrice, uint64_t buyPrice);

	int128_t 
	amountOfferedForSaleTimesSellPrice(uint64_t sellPrice, uint64_t buyPrice) const;

};

} /* stellar */