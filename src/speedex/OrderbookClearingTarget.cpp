#include "speedex/OrderbookClearingTarget.h"

#include "speedex/IOCOffer.h"
#include "ledger/LedgerTxn.h"

#include "transactions/TransactionUtils.h"

#include "ledger/TrustLineWrapper.h"

namespace stellar {

OrderbookClearingTarget::OrderbookClearingTarget(
	AssetPair tradingPair, uint64_t sellPrice, uint64_t buyPrice, int128_t totalClearingTarget)
	: mTradingPair(tradingPair)
	, mSellPrice(sellPrice)
	, mBuyPrice(buyPrice)
	, mTotalClearTarget(totalClearingTarget) {}

// n/d <= sell/buy  ==> n * buy <= d * sell
bool
OrderbookClearingTarget::checkPrice(const IOCOffer& offer) {
	int128_t lhs = ((int128_t)offer.mMinPrice.n) * mBuyPrice;
	int128_t rhs = ((int128_t)offer.mMinPrice.d) * mSellPrice;
	return lhs <= rhs;
}


int64_t 
OrderbookClearingTarget::getSellAmount(int128_t amountTimesPrice) const {
	return (amountTimesPrice / mSellPrice) + (amountTimesPrice % mSellPrice == 0 ? 0 : 1); // ceil

}
int64_t 
OrderbookClearingTarget::getBuyAmount(int128_t amountTimesPrice) const {
	return amountTimesPrice / mBuyPrice;
}

SpeedexOfferClearingStatus
OrderbookClearingTarget::clearOffer(AbstractLedgerTxn& ltx, const IOCOffer& offer) {

	if (!checkPrice(offer)) {
		throw std::logic_error("tried to clear offer with bad price!");
	}

	int128_t curSellRealization = std::min(mTotalClearTarget - mRealizedClearTarget, ((int128_t)offer.mSellAmount) * ((int128_t)mSellPrice));

	mRealizedClearTarget += curSellRealization;

	int64_t sellAmount = getSellAmount(curSellRealization);
	int64_t buyAmount = getBuyAmount(curSellRealization);

	mRealizedSellAmount += sellAmount;
	mRealizedBuyAmount += buyAmount;

	auto header = ltx.loadHeader();

	auto doTransfer = [&] (Asset asset, int64_t amount) {
		if (asset.type() == ASSET_TYPE_NATIVE) {
			auto account = loadAccount(ltx, offer.mSourceAccount);
			auto ok = addBalance(header, account, amount);
			if (!ok) {
				throw std::runtime_error("commutative precondition fail when clearing offer");
			}
		} else {
	        auto sourceLine = loadTrustLine(ltx, offer.mSourceAccount, asset);

	        if (!sourceLine) {
	        	throw std::runtime_error("commutative precondition fail when clearing offer");
	        }

	        if (!sourceLine.addBalance(header, amount))
	        {
	            throw std::runtime_error("commutative preconditions when clearing offer");
	        }
		}
	};

	doTransfer(mTradingPair.buying, buyAmount);
	//When creating the offer, we do not modify account balances.
	//The correct approach might instead to be adjust an account's liabilities during offer
	//creation instead.
	doTransfer(mTradingPair.selling, -sellAmount);

	return offer.getClearingStatus(sellAmount, buyAmount, mTradingPair);
}

SpeedexLiquidityPoolClearingStatus
OrderbookClearingTarget::finishWithLiquidityPool(AbstractLedgerTxn& ltx, LiquidityPoolFrame& lpFrame) {
	int128_t remainingToClear = mTotalClearTarget - mRealizedClearTarget;

	int64_t sellAmount = getSellAmount(remainingToClear);
	int64_t buyAmount = getBuyAmount(remainingToClear);

	lpFrame.assertValidSellAmount(sellAmount, mSellPrice, mBuyPrice);

	auto status = lpFrame.doTransfer(sellAmount, buyAmount, mSellPrice, mBuyPrice);

	mRealizedSellAmount += sellAmount;
	mRealizedBuyAmount += buyAmount;

	mRealizedClearTarget += remainingToClear;

	return status;
}

bool
OrderbookClearingTarget::doneClearing() const {
	return mTotalClearTarget == mRealizedClearTarget;
}

AssetPair 
OrderbookClearingTarget::getAssetPair() const {
	return mTradingPair;
}


} /* stellar */