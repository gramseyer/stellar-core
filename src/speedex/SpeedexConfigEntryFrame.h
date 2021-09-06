#pragma once

#include "xdr/Stellar-ledger-entries.h"
#include "ledger/AssetPair.h"

#include "speedex/TatonnementControls.h"

#include "util/XDROperators.h"

#include <memory>

namespace stellar {

class SpeedexConfigSnapshotFrame {
	std::shared_ptr<const LedgerEntry> mSpeedexConfig;

public:

	SpeedexConfigSnapshotFrame(std::shared_ptr<const LedgerEntry> config);

	operator bool() const {
		return (bool) mSpeedexConfig;
	}

	bool isValidAssetPair(const AssetPair& tradingPair) const;

	std::vector<Asset> getAssets() const;

	TatonnementControlParams getControls() const;

	std::map<Asset, uint64_t> getStartingPrices() const;
};


} /* stellar */