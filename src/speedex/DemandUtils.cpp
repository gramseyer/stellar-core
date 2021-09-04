#include "speedex/DemandUtils.h"

namespace stellar
{

void
SupplyDemand::addSupplyDemand(Asset const& sell, Asset const& buy, int128_t amount)
{
	mSupplyDemand[sell].first += amount;
	mSupplyDemand[buy].second += amount;
}
void
SupplyDemand::addSupplyDemand(AssetPair const& tradingPair, int128_t amount) 
{
	addSupplyDemand(tradingPair.selling, tradingPair.buying, amount);
}

SupplyDemand::int128_t
SupplyDemand::getDelta(Asset const& asset) const {
	auto const& [supply, demand] = mSupplyDemand.at(asset);
	return demand - supply;
}

TatonnementObjectiveFn 
SupplyDemand::getObjective() const
{
	return TatonnementObjectiveFn(mSupplyDemand);
}

TatonnementObjectiveFn::TatonnementObjectiveFn(std::map<Asset, std::pair<int128_t, int128_t>> const& supplyDemands)
	: value({0, 0})
{
	for (auto const& [_, sd] : supplyDemands)
	{
		auto const& [supply, demand] = sd;
		value += uint256_t::square(demand-supply);
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

} /* stellar */