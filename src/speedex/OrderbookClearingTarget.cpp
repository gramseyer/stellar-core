#include "speedex/OrderbookClearingTarget.h"

#include "speedex/IOCOffer.h"
#include "ledger/LedgerTxn.h"

#include "transactions/TransactionUtils.h"

#include "ledger/TrustLineWrapper.h"

#include "util/types.h"

namespace stellar {

OrderbookClearingTarget::OrderbookClearingTarget(
	AssetPair tradingPair, uint64_t sellPrice, uint64_t buyPrice, int128_t totalClearingTarget)
	: mTradingPair(tradingPair)
	, mSellPrice(sellPrice)
	, mBuyPrice(buyPrice)
	, mTotalClearTarget(totalClearingTarget)
	, mRealizedClearTarget(0)
	, mRealizedSellAmount(0)
	, mRealizedBuyAmount(0) {}

// n/d <= sell/buy  ==> n * buy <= d * sell
bool
OrderbookClearingTarget::checkPrice(const IOCOffer& offer) {
	int128_t lhs = ((int128_t)offer.mMinPrice.n) * mBuyPrice;
	int128_t rhs = ((int128_t)offer.mMinPrice.d) * mSellPrice;
	return lhs <= rhs;
}


int64_t 
OrderbookClearingTarget::getSellAmount(int128_t amountTimesPrice) const {
	return (amountTimesPrice / ((int128_t) mSellPrice)) + (amountTimesPrice % ((int128_t) mSellPrice) == 0 ? 0 : 1); // ceil

}
int64_t 
OrderbookClearingTarget::getBuyAmount(int128_t amountTimesPrice) const {
	return amountTimesPrice / mBuyPrice;
}

void
OrderbookClearingTarget::print() const
{
	auto sell = assetToString(mTradingPair.selling);
	auto buy = assetToString(mTradingPair.buying);
	std::printf("sell %s buy %s target %lf\n", sell.c_str(), buy.c_str(), (double) mTotalClearTarget);
}

SpeedexOfferClearingStatus
OrderbookClearingTarget::clearOffer(AbstractLedgerTxn& ltx, const IOCOffer& offer) {

	if (!checkPrice(offer)) {
		throw std::logic_error("tried to clear offer with bad price!");
	}
	std::printf("clearing an offer with amount %lld sellPrice (%ld / %ld) \n", offer.mSellAmount, offer.mMinPrice.n, offer.mMinPrice.d);

	int128_t offeredSellRealization = static_cast<int128_t>(offer.mSellAmount) * static_cast<int128_t>(mSellPrice);

	//std::printf("offeredSellRealization (double %lf) %lld\n", (double) offeredSellRealization, (int64_t) offeredSellRealization);

	int128_t curSellRealization = std::min(mTotalClearTarget - mRealizedClearTarget, offeredSellRealization);

	mRealizedClearTarget += curSellRealization;

	//std::printf("curSellRealization (double %lf) %lld\n", (double) curSellRealization, (int64_t) curSellRealization);

	int64_t sellAmount = getSellAmount(curSellRealization);
	int64_t buyAmount = getBuyAmount(curSellRealization);

	//std::printf("sellAmount %lld buyAmount %lld\n", sellAmount, buyAmount);

	mRealizedSellAmount += sellAmount;
	mRealizedBuyAmount += buyAmount;

	auto header = ltx.loadHeader();

	auto doTransfer = [&] (Asset asset, int64_t amount) {
		if (asset.type() == ASSET_TYPE_NATIVE) {
			auto account = loadAccount(ltx, offer.mSourceAccount);
			auto ok = addBalance(header, account, amount);
			if (!ok) {
				throw std::runtime_error("fail to add xlm balance");
			}
		} else {
	        auto sourceLine = loadTrustLine(ltx, offer.mSourceAccount, asset);

	        if (!sourceLine) {
	        	throw std::runtime_error("failed to find trustline");
	        }

	        if (!sourceLine.addBalance(header, amount))
	        {
	            throw std::runtime_error("failed to add nonxlm balance");
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
	//std::printf("mTotalClearTarget (double %lf) %lld\n", (double) mTotalClearTarget, (int64_t) mTotalClearTarget);
	//std::printf("mRealizedClearTarget (double %lf) %lld\n", (double) mRealizedClearTarget, (int64_t) mRealizedClearTarget);


	int128_t remainingToClear = mTotalClearTarget - mRealizedClearTarget;

	//std::printf("remainingToClear (double %lf) %lld\n", (double) remainingToClear, (int64_t) remainingToClear);

	int64_t sellAmount = getSellAmount(remainingToClear);
	int64_t buyAmount = getBuyAmount(remainingToClear);

	//std::printf("sellAmount %lld buyAmount %lld\n", sellAmount, buyAmount);

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