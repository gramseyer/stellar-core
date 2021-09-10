#include "speedex/TatonnementControls.h"

#include "speedex/DemandUtils.h"

namespace stellar
{


TatonnementControlParamsWrapper::TatonnementControlParamsWrapper(
	TatonnementControlParams const& params)
	: mParams(params)
	, kPriceMin(1)
	, kPriceMax(UINT64_MAX >> (mParams.mSmoothMult + 1))
	, kMinStepSize((1llu) << (mParams.mStepSizeRadix + 1))
	, kStartingStepSize(kMinStepSize)
	{};

void
TatonnementControlParamsWrapper::incrementRound()
{
	mRoundNumber ++;
}

bool
TatonnementControlParamsWrapper::done() const
{
	return mRoundNumber >= mParams.mMaxRounds;
}

uint64_t 
TatonnementControlParamsWrapper::stepUp(uint64_t step) const
{
	uint64_t out = (step * mParams.mStepUp) >> mParams.mStepSizeRadix;
	if (out < step) {
		//overflow
		return step;
	}
	return out;
}

uint64_t 
TatonnementControlParamsWrapper::stepDown(uint64_t step) const
{
	uint64_t out = step * mParams.mStepDown;
	if (out < step)
	{
		//overflow
		return (step >> mParams.mStepSizeRadix) * mParams.mStepDown;
	}
	return out >> mParams.mStepSizeRadix;
}

uint64_t 
TatonnementControlParamsWrapper::imposePriceBounds(uint64_t candidate) const {
	if (candidate > kPriceMax) {
		return kPriceMax;
	}
	if (candidate < kPriceMin) {
		return kPriceMin;
	}
	return candidate;
}

uint64_t 
TatonnementControlParamsWrapper::setTrialPrice(uint64_t curPrice, int128_t const& demand, uint64_t stepSize) const
{
	/*

	p -> p * (1 + demand * step), where demand is (already) weighted by pricel

	p += p * demand * step

	the delta is price * demand * stepSize >> stepRadix;

	*/


	uint128_t step_times_old_price = ((uint128_t) curPrice) * ((uint128_t) stepSize);

	std::printf("%lf\n", (double) step_times_old_price);
	
	// want step_times_old_price * demand >> mParams.mStepRadix;

	int sign = (demand > 0) ? 1 : -1;

	uint128_t unsignedDemand = demand > 0 ? demand : -demand;

	std::printf("%lf %lf\n", (double) demand, (double) unsignedDemand);

	uint256_t product = uint256_t::product(step_times_old_price, unsignedDemand);

	int64_t delta = (product.compress(mParams.mStepRadix));

	std::printf("delta = %llx sign = %d\n", delta, sign);

	uint64_t candidateOut;
	if (sign > 0) {
		candidateOut = curPrice + delta;
		if (candidateOut < curPrice)
		{
			candidateOut = UINT64_MAX;
		}
	} else {
		if (curPrice > delta) {
			candidateOut = curPrice - delta;
		} else {
			candidateOut = 0;
		}
	}
	return imposePriceBounds(candidateOut);
}



std::map<Asset, uint64_t>
TatonnementControlParamsWrapper::setTrialPrices(
	std::map<Asset, uint64_t> const& curPrices, SupplyDemand const& demands, uint64_t stepSize) const
{
	std::map<Asset, uint64_t> pricesOut;
	for (auto const& [asset, curPrice] : curPrices)
	{
		pricesOut[asset] = setTrialPrice(curPrice, demands.getDelta(asset), stepSize);
	}

	return pricesOut;
}


} /* stellar */