#include "speedex/IOCOrderbook.h"

#include "ledger/LedgerTxn.h"

namespace stellar {

IOCOrderbook::IOCOrderbook(AssetPair tradingPair) 
	: mTradingPair(tradingPair)
	, mCleared(false)
	{};


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

bool
priceNEQ(Price const& p1, Price const& p2)
{
	return ((uint64_t)p1.n) * ((uint64_t) p2.d) != ((uint64_t)p1.d) * ((uint64_t) p2.n);
}

void 
IOCOrderbook::doPriceComputationPreprocessing() {
	std::printf("preprocess\n");
	PriceCompStats stats = zeroStats;
	mPrecomputedTatonnementData.clear();
	for (auto& offer : mOffers) {
		//intentionally starting with 0 at bot
		if (priceNEQ(offer.mMinPrice, stats.marginalPrice))
		{
			std::printf("adding (%d / %d) %lld\n", stats.marginalPrice.n, stats.marginalPrice.d, stats.cumulativeOfferedForSale);
			mPrecomputedTatonnementData.push_back(stats);
		}
		stats.marginalPrice = offer.mMinPrice;
		stats.cumulativeOfferedForSale += offer.mSellAmount;

		int128_t offerTimesPrice = ((int128_t) offer.mSellAmount) * offer.mMinPrice.n;
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

bool priceLT(Price const& p, uint64_t sellPrice, uint64_t buyPrice) {
	using int128_t = __int128_t;

	std::printf("priceLT comparison: sell %lu buy %lu p.n %d p.d %d\n", sellPrice, buyPrice, p.n, p.d);

	// p.n / p.d <? sellPrice / buyPrice
	return (((int128_t) p.n) * ((int128_t) buyPrice)) < (((int128_t) p.d) * ((int128_t) sellPrice)); 
}

bool priceLTE(Price const& p, uint64_t sellPrice, uint64_t buyPrice) {
	using int128_t = __int128_t;

	std::printf("priceLTE comparison: sell %lu buy %lu p.n %d p.d %d\n", sellPrice, buyPrice, p.n, p.d);


	// p.n / p.d <=? sellPrice / buyPrice
	return (((int128_t) p.n) * ((int128_t) buyPrice)) <= (((int128_t) p.d) * ((int128_t) sellPrice)); 
}

IOCOrderbook::PriceCompStats 
IOCOrderbook::getPriceCompStats(uint64_t sellPrice, uint64_t buyPrice) const {


	for (auto const& stats : mPrecomputedTatonnementData)
	{
		std::printf("marginal p (%d / %d) amount %ld\n", stats.marginalPrice.n, stats.marginalPrice.d, stats.cumulativeOfferedForSale);
	}

	if (mPrecomputedTatonnementData.size() == 1)
	{
		return zeroStats;
	}

	size_t start = 1;
	size_t end = mPrecomputedTatonnementData.size() - 1;

	if (priceLTE(mPrecomputedTatonnementData[end].marginalPrice, sellPrice, buyPrice))
	{
		std::printf("larger than end\n");
		return mPrecomputedTatonnementData[end];
	}

	std::printf("start bin search\n");

	while (true) {
		size_t mid = (start + end) / 2;


		std::printf("start = %lu end = %lu mid= %lu \n", start, end, mid);

		if (start == end)
		{
			return mPrecomputedTatonnementData[start - 1];
		}

		if (priceLTE(mPrecomputedTatonnementData[mid].marginalPrice, sellPrice, buyPrice))
		{
			start = mid + 1;
		} else {
			end = mid;
		}
	}
}




} /* stellar */