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

struct TatonnementControlParams
{
	uint8_t mTaxRate, mSmoothMult;

	uint32_t mMaxRounds;

	// step -> stepUp * step / (2^mStepSizeRadix);
	uint8_t mStepUp, mStepDown, mStepSizeRadix;

	uint8_t mStepRadix;
};

struct SupplyDemand;

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
	setTrialPrices(std::map<Asset, uint64_t> const& curPrices, SupplyDemand const& demands, uint64_t stepSize) const;

	uint64_t imposePriceBounds(uint64_t candidatePrice) const;

	uint32_t getRoundNumber() const {
		return mRoundNumber;
	}
};



} /* stellar */