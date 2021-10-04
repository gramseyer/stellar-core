#include "speedex/IOCOrderbook.h"

#include "ledger/LedgerTxn.h"

#include "util/types.h"

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
	//std::printf("preprocess\n");
	PriceCompStats stats = zeroStats;
	mPrecomputedTatonnementData.clear();
	for (auto& offer : mOffers) {
		//intentionally starting with 0 at bot
		if (priceNEQ(offer.mMinPrice, stats.marginalPrice))
		{
		//	std::printf("adding (%d / %d) %lld\n", stats.marginalPrice.n, stats.marginalPrice.d, stats.cumulativeOfferedForSale);
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

std::pair<std::vector<SpeedexOfferClearingStatus>, std::optional<SpeedexLiquidityPoolClearingStatus>>
IOCOrderbook::clearOffers(AbstractLedgerTxn& ltx, OrderbookClearingTarget& target, LiquidityPoolFrame& lpFrame)
{
	throwIfCleared();

	auto sellStr = assetToString(mTradingPair.selling);
	auto buyStr = assetToString(mTradingPair.buying);

	std::printf("clearing trade pair sell %s buy %s\n", sellStr.c_str(), buyStr.c_str());

	std::vector<SpeedexOfferClearingStatus> out;

	for (auto iter = mOffers.begin(); iter != mOffers.end(); iter++) {
		if (!target.doneClearing()) {
			// TODO adjust here if prioritizing full execution over trading at all
			out.push_back(target.clearOffer(ltx, *iter));
		} else
		{
			break;
		}
	}

	std::optional<SpeedexLiquidityPoolClearingStatus> lpRes = std::nullopt;

	if (!target.doneClearing() && lpFrame) {
		std::printf("finishing with lp\n");
		lpRes = target.finishWithLiquidityPool(ltx, lpFrame);
	}
	if (!target.doneClearing()) {
		throw std::runtime_error("invalid trade amounts!");
	}
	mCleared = true;

	return {out, lpRes};
}

void
IOCOrderbook::finish() {

	if (!mCleared) {

		mCleared = true;
	}
}

bool priceLT(Price const& p, uint64_t sellPrice, uint64_t buyPrice) {
	using int128_t = __int128_t;

	//std::printf("priceLT comparison: sell %lu buy %lu p.n %d p.d %d\n", sellPrice, buyPrice, p.n, p.d);

	// p.n / p.d <? sellPrice / buyPrice
	return (((int128_t) p.n) * ((int128_t) buyPrice)) < (((int128_t) p.d) * ((int128_t) sellPrice)); 
}

bool priceLTE(Price const& p, uint64_t sellPrice, uint64_t buyPrice) {
	using int128_t = __int128_t;

	//std::printf("priceLTE comparison: sell %lu buy %lu p.n %d p.d %d\n", sellPrice, buyPrice, p.n, p.d);


	// p.n / p.d <=? sellPrice / buyPrice
	return (((int128_t) p.n) * ((int128_t) buyPrice)) <= (((int128_t) p.d) * ((int128_t) sellPrice)); 
}

IOCOrderbook::PriceCompStats 
IOCOrderbook::getPriceCompStats(uint64_t sellPrice, uint64_t buyPrice) const {


	//for (auto const& stats : mPrecomputedTatonnementData)
	//{
	//	std::printf("marginal p (%d / %d) amount %ld\n", stats.marginalPrice.n, stats.marginalPrice.d, stats.cumulativeOfferedForSale);
	//}

	if (mPrecomputedTatonnementData.size() == 1)
	{
		return zeroStats;
	}

	size_t start = 1;
	size_t end = mPrecomputedTatonnementData.size() - 1;

	if (priceLTE(mPrecomputedTatonnementData[end].marginalPrice, sellPrice, buyPrice))
	{
	//	std::printf("larger than end\n");
		return mPrecomputedTatonnementData[end];
	}

	//std::printf("start bin search\n");

	while (true) {
		size_t mid = (start + end) / 2;


		//std::printf("start = %lu end = %lu mid= %lu \n", start, end, mid);

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

uint64_t applySmoothMult(uint64_t sellPrice, uint8_t smoothMult) {
	return smoothMult == 0 ? sellPrice : sellPrice - (sellPrice >> smoothMult);
}

IOCOrderbook::int128_t 
IOCOrderbook::cumulativeOfferedForSaleTimesPrice(uint64_t sellPrice, uint64_t buyPrice, uint8_t smoothMult) const
{

	/*
	p = (sellPrice / buyPrice)

	An offer executes fully if offer.minPrice < p * (1-2^{smoothMult}), 
			 executes not at all if offer.minPrice > p,
	     and executes x-fractionally in the interim

	     for x = (p - offer.minPrice) / (p * 2^smoothMult);

	Note that x\in[0,1] and offer execution is a continuous function.


	Considering only offers in the interim gap:

	amount sold = \Sigma_i ((p-offers[i].mP) / (p * 2^smoothMult)) * offer.amount
				= \Sigma_i (offer.amount) / (2^smoothMult) - \Sigma_i (offer.amount * offer.mP) / (p * 2^smoothMult)

	amount sold * sellPrice (== amount bought * buyPrice)
		= 2^{-smoothMult} * (
				\Sigma_i offer.amount * sellPrice
			  - \Sigma_i offer.amount * offer.mP * sellPrice / (sellPrice / buyPrice)
			)
		= 2^{-smoothMult} * (
				\Sigma_i offer.amount * sellPrice
			  - \Sigma_i offer.amount * offer.mP * buyPrice
			)


	Since smooth_mult division can be done with just a bitshift, his avoids any divisions!

	Hence, we precompute answers to queries of the form
		* \Sigma_{i : offer[i].mp < p} offer[i].amount
		* \Sigma_{i : offer[i].mp < p} offer[i].amount * offer[i].mP

	Note that offer.mP < sellPrice / buyPrice, for these offers.

	Hence, offer.mP * buyPrice < sellPrice

	How wide do the intermediate results need to be?

	\Sigma_i offer[i].amount < INT64_MAX by assumption.


	Stellar's notion of price is (32 bits) / (32 bits).

	Tatonnement *might* be easier with wider prices internally, so we have sellPrice and buyPrice as 64 bits each.

	\Sigma_i offer[i].amount * sellPrice fits within INT128_MAX.
	\Sigma_i offer[i].amount * offer[i].mP fits with no (well, essentially no, people who put non-dyadic p.d are weird) precision loss 
		if we represent query answers as 128 bits (96 bits . 32 bits)

		However, by the assumption above, multiplying this by buyPrice is always safe, if we drop the low 32 bits during the multiplication.
		Open question is precision loss there.  Is it worth expanding beyond 128 bits internally within tatonnement?

	Of course, shifting everything up by smoothMult bits breaks this safety.

	As such, we need to constraint sell/buyPrice to be in [1, PRICE_MAX] with ceil(log_2(PRICE_MAX)) + smoothMult < 64;

	*/

	auto partialExecSellPrice = sellPrice;
	auto fullExecSellPrice = applySmoothMult(sellPrice, smoothMult);

	auto partialExecStats = getPriceCompStats(partialExecSellPrice, buyPrice);
	auto fullExecStats = getPriceCompStats(fullExecSellPrice, buyPrice);

	int64_t fullExecEndow = fullExecStats.cumulativeOfferedForSale;
	int64_t partialExecEndow = partialExecStats.cumulativeOfferedForSale - fullExecEndow;

//	std::printf("full endow %lld partial endow %lld\n", fullExecEndow, partialExecEndow);

	int128_t fullExecEndowTimesPrice = fullExecStats.cumulativeOfferedForSaleTimesPrice;
	int128_t partialExecEndowTimesPrice = partialExecStats.cumulativeOfferedForSaleTimesPrice - fullExecEndowTimesPrice;

	int128_t partialAmountTimesSellPrice = ((int128_t) partialExecEndow) * ((int128_t) sellPrice);

	constexpr auto wideMultShiftDown = [] (int128_t radix_val, uint64_t mult) -> int128_t {
		int128_t lowbits = radix_val & PriceCompStats::OFFERED_TIMES_PRICE_LOWBITS_MASK;
		int128_t highbits = radix_val >> PriceCompStats::OFFERED_TIMES_PRICE_RADIX;
		return (highbits * mult) + ((lowbits * mult) >> PriceCompStats::OFFERED_TIMES_PRICE_RADIX);
	};

	//std::printf("partialExecEndowTimesPrice %lf\n", (double) partialExecEndowTimesPrice / (double) 0x100000000);

	int128_t partialAmountTimesMinPriceTimesBuyPrice
		= wideMultShiftDown(partialExecEndowTimesPrice, buyPrice);

	//std::printf("partialAmountTimesMinPriceTimesBuyPrice %lf\n", (double) partialAmountTimesMinPriceTimesBuyPrice);
	//std::printf("partialAmountTimesSellPrice %lf\n", (double) partialAmountTimesSellPrice);

	int128_t partialAmountSum = partialAmountTimesSellPrice - partialAmountTimesMinPriceTimesBuyPrice;

	int128_t valueSoldPartialExec = partialAmountSum << smoothMult;

	int128_t valueSoldFullExec = ((int128_t) fullExecEndow) * ((int128_t) sellPrice);

	//std::printf("valueSoldFullExec %lf valueSoldPartialExec %lf\n", (double) valueSoldFullExec, (double) valueSoldPartialExec);

	return valueSoldFullExec + valueSoldPartialExec;
}





} /* stellar */