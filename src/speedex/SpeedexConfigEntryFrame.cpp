#include "speedex/SpeedexConfigEntryFrame.h"

#include "util/XDROperators.h"

namespace stellar {

SpeedexConfigSnapshotFrame::SpeedexConfigSnapshotFrame(std::shared_ptr<const LedgerEntry> config)
	: mSpeedexConfig(config) {
		if (!config) {
			mSpeedexConfig = nullptr;
			return;
		}

		if (config -> data.type() != SPEEDEX_CONFIG) {
			mSpeedexConfig = nullptr;
		}
	}

bool 
SpeedexConfigSnapshotFrame::isValidAssetPair(const AssetPair& tradingPair) const
{
	auto const& assets = mSpeedexConfig -> data.speedexConfig().speedexAssets;
	bool foundBuying = false, foundSelling = false;
	for (auto const& asset : assets) {
		if (tradingPair.selling == asset) {
			foundSelling = true;
		}
		if (tradingPair.buying == asset) {
			foundBuying = true;
		}
	}
	return (foundSelling && foundBuying && (tradingPair.selling != tradingPair.buying));
}

std::vector<Asset> 
SpeedexConfigSnapshotFrame::getAssets() const
{
	return mSpeedexConfig->data.speedexConfig().speedexAssets;
}

TatonnementControlParams
SpeedexConfigSnapshotFrame::getControls() const
{
	return TatonnementControlParams
    {
        .mTaxRate = 5,
        .mSmoothMult = 5,
        .mMaxRounds = 1000,
        .mStepUp = 45,
        .mStepDown = 25,
        .mStepSizeRadix = 5,
        .mStepRadix = 65
    };
}

std::map<Asset, uint64_t>
SpeedexConfigSnapshotFrame::getStartingPrices() const
{
	std::map<Asset, uint64_t> prices;
	for (auto const& asset : getAssets())
	{
		prices[asset] = 0x100000000;
	}
	return prices;
}

} /* stellar */