#pragma once

namespace stellar {

class AbtractLedgerTxn;
class IOCOffer;
class LiquidityPoolFrame;

class OrderbookClearingTarget {
	using uint128_t = unsigned __int128_t;
	using int128_t = __int128_t;

	const AssetPair mTradingPair;

	const uint64_t mSellPrice, mBuyPrice;

	int128_t mTotalClearTarget;
	int128_t mRealizedClearTarget;

	int64_t mRealizedSellAmount, mRealizedBuyAmount;

	bool checkPrice(const IOCOffer& offer);


	//bool offerWouldClearFully(const IOCOffer& offer) const;

	bool doneClearing() const;

	int64_t getSellAmount(int128_t amountTimesPrice, uint64_t sellPrice) const;
	int64_t getBuyAmount(int128_t amountTimesPrice, uint64_t buyPrice) const;

public:

	int64_t getRealizedSellAmount() const {
		return mRealizedSellAmount;
	}
	int64_t getRealizedBuyAmount() const {
		return mRealizedBuyAmount;
	}

	OrderbookClearingTarget(AssetPair tradingPair, uint64_t sellPrice, uint64_t buyPrice, uint128_t totalClearingTarget);

	void clearOffer(AbstractLedgerTxn& ltx, const IOCOffer& offer);

	AssetPair getAssetPair() const;


	//void clearOffers(IOCOrderbook& offers, LiquidityPoolFrame& lp);

};

}