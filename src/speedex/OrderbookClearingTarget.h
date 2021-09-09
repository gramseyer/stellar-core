#pragma once

#include "ledger/AssetPair.h"
#include <cstdint>
#include "xdr/Stellar-ledger.h"

namespace stellar {

class AbstractLedgerTxn;
struct IOCOffer;
class LiquidityPoolFrame;

class OrderbookClearingTarget {
	using int128_t = __int128_t;

	const AssetPair mTradingPair;

	const uint64_t mSellPrice, mBuyPrice;

	int128_t mTotalClearTarget;
	int128_t mRealizedClearTarget;

	int64_t mRealizedSellAmount, mRealizedBuyAmount;

	bool checkPrice(const IOCOffer& offer);

	int64_t getSellAmount(int128_t amountTimesPrice) const;
	int64_t getBuyAmount(int128_t amountTimesPrice) const;

public:


	void print() const;
	
	int64_t getRealizedSellAmount() const {
		return mRealizedSellAmount;
	}
	int64_t getRealizedBuyAmount() const {
		return mRealizedBuyAmount;
	}

	OrderbookClearingTarget(AssetPair tradingPair, uint64_t sellPrice, uint64_t buyPrice, int128_t totalClearingTarget);

	SpeedexOfferClearingStatus
	clearOffer(AbstractLedgerTxn& ltx, const IOCOffer& offer);

	AssetPair getAssetPair() const;

	bool doneClearing() const;

	SpeedexLiquidityPoolClearingStatus
	finishWithLiquidityPool(AbstractLedgerTxn& ltx, LiquidityPoolFrame& lpFrame);
};

}