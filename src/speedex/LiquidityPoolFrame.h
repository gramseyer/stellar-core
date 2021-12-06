#pragma once

#include "ledger/LedgerTxnEntry.h"
#include "ledger/AssetPair.h"

namespace stellar {

class AbstractLedgerTxn;
class LiquidityPoolFrameBase;

class LiquidityPoolFrame {

	using int128_t = __int128;

	LiquidityPoolFrameBase& mBaseFrame;

	AssetPair mTradingPair;

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

	LiquidityPoolFrame(LiquidityPoolFrameBase& baseFrame, AssetPair const& tradingPair);
	
	operator bool() const;

	int64_t amountOfferedForSale(uint64_t sellPrice, uint64_t buyPrice) const;

	void assertValidSellAmount(int64_t sellAmount, uint64_t sellPrice, uint64_t buyPrice) const;

	void assertValidTrade(int64_t sellAmount, int64_t buyAmount, uint64_t sellPrice, uint64_t buyPrice) const;

	SpeedexLiquidityPoolClearingStatus 
	doTransfer(int64_t sellAmount, int64_t buyAmount, uint64_t sellPrice, uint64_t buyPrice);

	int128_t 
	amountOfferedForSaleTimesSellPrice(uint64_t sellPrice, uint64_t buyPrice) const;
};

} /* stellar */