#include "speedex/TatonnementControls.h"

namespace stellar
{

uint256_t 
uint256_t::square(int128_t a)
{
	constexpr uint128_t lowbits_mask = UINT64_MAX;
	//constexpr uint128_t highbits_mask = lowbits_mask << 64;

	a = a < 0 ? -a : a;

	const uint128_t lowA = a & lowbits_mask;
	const uint128_t highA = a >> 64;

	uint128_t highOut = 0, lowOut = 0;

	highOut += highA * highA;

	lowOut += lowA * lowA;

	uint128_t mid = lowA * highA;

	uint128_t midLow = mid & lowbits_mask;
	uint128_t midHigh = mid >> 64;

	highOut += 2 * midHigh;

	midLow *= 2;
	if (midLow > lowbits_mask)
	{
		highOut++;
		midLow &= lowbits_mask;
	}

	uint128_t candidateLow = lowOut + (midLow << 64);
	if (candidateLow < lowOut)
	{
		//overflow
		highOut++;
	}
	lowOut = candidateLow;

	return uint256_t {
		.lowbits = lowOut,
		.highbits = highOut
	};
}

uint256_t
uint256_t::product(uint128_t a, uint128_t b)
{
	constexpr auto lowbits = [] (uint128_t val) -> uint128_t {
		constexpr uint128_t lowbits_mask = UINT64_MAX;
		return val & lowbits_mask;
	};
	constexpr auto highbits = [] (uint128_t val) -> uint128_t {
		return val >> 64;
	};

	const uint128_t lowA = lowbits(a);
	const uint128_t lowB = lowbits(b);
	const uint128_t highA = highbits(a);
	const uint128_t highB = highbits(b);

	uint128_t highOut = highA * highB;
	uint128_t lowOut = lowA * lowB;

	auto addMidBits = [&] (uint128_t l, uint128_t r) -> void
	{
		uint128_t prod = l * r;
		uint128_t prodLow = lowbits(prod);
		uint128_t prodHigh = highbits(prod);
		
		highOut += prodHigh;
		
		prodLow <<= 64;

		uint128_t candidate = lowOut + prodLow;
		if (candidate < lowOut) {
			//overflow
			highOut ++;
		}
		lowOut = candidate;
	};
	addMidBits(lowA, highB);
	addMidBits(lowB, highA);

	return uint256_t {
		.lowbits = lowOut,
		.highbits = highOut
	};
}

int64_t 
uint256_t::compress(uint8_t radix) const 
{
	uint128_t out;
	if (radix < 128)
	{
		out = (lowbits >> radix) & (highbits << (128 - radix));
	} 
	else
	{
		out = (highbits >> (radix - 128));
	}
	if (out >= INT64_MAX)
	{
		return (int64_t) INT64_MAX;
	}
	return (int64_t) out;
}


uint256_t& 
uint256_t::operator+=(uint256_t const& other)
{
	highbits += other.highbits;
	uint128_t newLow = lowbits + other.lowbits;
	if (newLow < lowbits)
	{
		highbits++;
	}
	lowbits = newLow;
	return *this;
}

std::strong_ordering 
uint256_t::operator<=>(uint256_t const& other) const 
{
	if (highbits < other.highbits)
	{
		return std::strong_ordering::less;
	}
	if (highbits > other.highbits)
	{
		return std::strong_ordering::greater;
	}
	return lowbits <=> other.lowbits;
}


void 
uint256_t::subtract_smaller(const uint256_t& other)
{
	highbits -= other.highbits;
	uint128_t newLow = lowbits - other.lowbits;
	if (newLow > lowbits)
	{
		//overflow
		highbits--;
	}
	lowbits = newLow;
}

TatonnementObjectiveFn::TatonnementObjectiveFn(std::map<Asset, int128_t> const& excessDemands)
	: value({0, 0})
{
	for (auto const& [_, val] : excessDemands)
	{
		value += uint256_t::square(val);
	}
}

// is self <= other * (1+tolN/tolD)?
// is (self - other) < other * (tolN / tolD)
// this doesn't need to be super precise, so doing (other / tolD) * tolN is ok
bool 
TatonnementObjectiveFn::isBetterThan(TatonnementObjectiveFn const& other, uint8_t tolN, uint8_t tolD) const
{
	if (value <= other.value) return true;

	uint256_t compareL = value;
	compareL.subtract_smaller(other.value);
	uint256_t compareR {
		.lowbits = (other.value.lowbits / tolD) * tolN,
		.highbits = (other.value.highbits / tolD) * tolN
	};
	return compareL <= compareR;
}

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
	uint128_t step_times_old_price = ((uint128_t) curPrice) * ((uint128_t) stepSize);
	
	// want step_times_old_price * demand >> mParams.mStepRadix;

	int sign = (demand > 0) ? 1 : -1;

	uint128_t unsignedDemand = demand * sign;

	uint256_t product = uint256_t::product(step_times_old_price, unsignedDemand);

	int64_t delta = (product.compress(mParams.mStepRadix));

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
	std::map<Asset, uint64_t> const& curPrices, std::map<Asset, int128_t> const& demands, uint64_t stepSize) const
{
	std::map<Asset, uint64_t> pricesOut;
	for (auto const& [asset, curPrice] : curPrices)
	{
		pricesOut[asset] = setTrialPrice(curPrice, demands.at(asset), stepSize);
	}

	return pricesOut;
}


} /* stellar */