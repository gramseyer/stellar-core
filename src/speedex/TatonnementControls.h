#pragma once

#include <compare>
#include <map>
#include <cstdint>

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include "util/XDROperators.h"

#include "ledger/LedgerHashUtils.h"

namespace stellar
{

struct uint256_t
{
	using int128_t = __int128;
	using uint128_t = unsigned __int128;

	constexpr static uint128_t UINT128_MAX = ((uint128_t) 0) - 1;
	constexpr static uint128_t INT128_MAX = UINT128_MAX >> 1;

	uint128_t lowbits;
	uint128_t highbits;

	static uint256_t square(int128_t a);

	static uint256_t product(uint128_t a, uint128_t b);

	uint256_t& operator+=(uint256_t const& other);

	std::strong_ordering operator<=>(uint256_t const& other) const;

	bool operator==(uint256_t const& other) const = default;

	//requires that other <= this
	void subtract_smaller(const uint256_t& other);

	int64_t compress(uint8_t radix) const;
};

class TatonnementObjectiveFn
{

	using int128_t = __int128;

	uint256_t value;

public:

	TatonnementObjectiveFn(std::map<Asset, int128_t> const& excessDemands);

	// is self <= other * tolN/tolD?
	bool isBetterThan(TatonnementObjectiveFn const& other, uint8_t tolN, uint8_t tolD) const;
};

struct TatonnementControlParams
{
	uint8_t mTaxRate, mSmoothMult;

	uint32_t mMaxRounds;

	// step -> stepUp * step / (2^mStepSizeRadix);
	uint8_t mStepUp, mStepDown, mStepSizeRadix;

	uint8_t mStepRadix;
};

class TatonnementControlParamsWrapper
{
	TatonnementControlParams const& mParams;


	uint32_t mRoundNumber = 0;

	using int128_t = __int128;
	using uint128_t = unsigned __int128;

public:

	TatonnementControlParamsWrapper(TatonnementControlParams const& params);

	const uint64_t kPriceMin;
	const uint64_t kPriceMax;

	const uint64_t kMinStepSize;
	const uint64_t kStartingStepSize;

	uint64_t stepUp(uint64_t step) const;
	uint64_t stepDown(uint64_t step) const;

	void incrementRound();
	bool done() const;

	uint8_t smoothMult() const {
		return mParams.mSmoothMult;
	}
	uint8_t taxRate() const {
		return mParams.mTaxRate;
	}

	//todo relativizers?
	uint64_t setTrialPrice(uint64_t curPrice, int128_t const& demand, uint64_t stepSize) const;

	std::map<Asset, uint64_t>
	setTrialPrices(std::map<Asset, uint64_t> const& curPrices, std::map<Asset, int128_t> const& demands, uint64_t stepSize) const;

	uint64_t imposePriceBounds(uint64_t candidatePrice) const;
};



} /* stellar */