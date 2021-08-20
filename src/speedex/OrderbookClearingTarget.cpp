#include "speedex/OrderbookClearingTarget.h"

#include "speedex/IOCOffer.h"
#include "ledger/LedgerTxn.h"

namespace stellar {

OrderbookClearingTarget::OrderbookClearingTarget(
	AssetPair tradingPair, uint64_t sellPrice, uint64_t buyPrice, uint128_t totalClearingTarget)
	: mTradingPair(tradingPair)
	, mSellPrice(sellPrice)
	, mBuyPrice(buyPrice)
	, mTotalClearingTarget(totalClearingTarget) {}

// n/d <= sell/buy  ==> n * buy <= d * sell
bool
OrderbookClearingTarget::checkPrice(const IOCOffer& offer) {
	int128_t lhs = ((int128_t)offer.minPrice.n) * buyPrice;
	int128_t rhs = ((int128_t)offer.minPrice.d) * sellPrice;
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

void
OrderbookClearingTarget::clearOffer(AbstractLedgerTxn& ltx, const IOCOffer& offer) {

	if (!checkPrice(offer)) {
		throw std::logic_error("tried to clear offer with bad price!");
	}

	int128_t priorSellRealization = ((int128_t)mRealizedSellAmount) * ((int128_t)mSellPrice);
	int128_t curSellRealization = std::min(mTotalSellRealization - priorSellRealization, ((int128_t)offer.amount) * ((int128_t)sellPrice));

	mRealizedClearTarget += curSellRealization;

	int64_t sellAmount = getSellAmount(curSellRealization);
	int64_t buyAmount = getBuyAmount(curSellRealization);

	mRealizedSellAmount += sellAmount;
	mRealizedBuyAmount += buyAmount;

	auto header = ltx.loadHeader();

	auto doTransfer = [&] (Asset asset, int64_t amount) {
		if (amount < 0) {
			throw std::runtime_error("arithmetic error");
		}
		if (asset.type() == ASSET_TYPE_NATIVE) {
			auto ok = stellar::addBalance(header, sourceAccount, amount);
			if (!ok) {
				throw std::runtime_error("commutative precondition fail when clearing offer")
			}
		} else {
	        auto sourceLine = loadTrustLine(ltx, getSourceID(), asset);

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
	//When creating the offer, we reduced the account's balance my offer.mSellAmount.
	//The correct approach might instead to be adjust an account's liabilities during offer
	//creation instead.
	doTransfer(mTradingPair.selling, offer.mSellAmount - sellAmount);
}

void
OrderbookClearingTarget::finishWithLiquidityPool(AbstractLedgerTxn& ltx, LiquidityPoolFrame& lpFrame) {
	int128_t remainingToClear = mTotalClearTarget - mRealizedClearTarget;
	lpFrame.assertValidClearingAmount(remainingToClear, mSellPrice, mBuyPrice);

	int64_t sellAmount = getSellAmount(remainingToClear);
	int64_t buyAmount = getBuyAmount(remainingToClear);

	lpFrame.doTransfer(ltx, sellAmount, buyAmount);

	mRealizedSellAmount += sellAmount;
	mRealizedBuyAmount += buyAmount;

	mRealizedClearTarget += remainingToClear;
}

bool
OrderbookClearingTarget::doneClearing() const {
	return mTotalClearTarget == mRealizedClearTarget;
}

AssetPair 
OrderbookClearingTarget::getAssetPair() const {
	return mAssetPair;
}


} /* stellar */