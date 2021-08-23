#include "speedex/IOCOrderbook.h"

#include "ledger/LedgerTxn.h"

namespace stellar {

IOCOrderbook::IOCOrderbook(AssetPair tradingPair) : mTradingPair(tradingPair) {};


void
IOCOrderbook::throwIfCleared() {
	if (mCleared) {
		throw std::runtime_error("Throw if cleared!");
	}
}

void
IOCOrderbook::throwIfNotCleared() {
	if (!mCleared) {
		throw std::runtime_error("Throw if not cleared!");
	}
}

void 
IOCOrderbook::doPriceComputationPreprocessing() {
	PriceCompStats stats;
	for (auto& offer : mOffers) {
		mPrecomputedTatonnementData.push_back(stats); // intentionally starting with 0
		stats.marginalPrice = offer.mMinPrice;
		stats.cumulativeOfferedForSale += offer.mSellAmount;

		int128_t offerTimesPrice = offer.mSellAmount * offer.mMinPrice.n;
		offerTimesPrice <<= PriceCompStats::OFFERED_TIMES_PRICE_RADIX;

		stats.cumulativeOfferedForSaleTimesPrice += offerTimesPrice / offer.mMinPrice.d;
	}

	mPrecomputedTatonnementData.push_back(stats);
}

void 
IOCOrderbook::addOffer(IOCOffer offer) {
	mOffers.insert(offer);
}

void
IOCOrderbook::commitChild(const IOCOrderbook& other) {
	throwIfCleared();

	if (mTradingPair != other.mTradingPair) {
		throw std::runtime_error("merge orderbooks trading pair mismatch!");
	}
	mOffers.insert(other.mOffers.begin(), other.mOffers.end());
	mPrecomputedTatonnementData.clear();
}

void 
IOCOrderbook::clearOffers(AbstractLedgerTxn& ltx, OrderbookClearingTarget& target, LiquidityPoolFrame& lpFrame) {

	throwIfCleared();
	for (auto iter = mOffers.begin(); iter != mOffers.end(); iter++) {
		if (!target.doneClearing()) {
			target.clearOffer(ltx, *iter);
		} else {
			iter -> unwindOffer(ltx, mTradingPair.selling);
		}
	}

	if (!target.doneClearing() && lpFrame) {
		target.finishWithLiquidityPool(ltx, lpFrame);
	}
	if (!target.doneClearing()) {
		throw std::runtime_error("invalid trade amounts!");
	}
	mCleared = true;
}

void
IOCOrderbook::finish(AbstractLedgerTxn& ltx) {

	if (!mCleared) {

		for (auto const& offer : mOffers) {
			offer.unwindOffer(ltx, mTradingPair.selling);
		}

		mCleared = true;
	}
}



} /* stellar */