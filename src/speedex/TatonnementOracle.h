#pragma once
#include "speedex/TatonnementControls.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include "util/XDROperators.h"

#include "ledger/LedgerHashUtils.h"

#include  <cstdint>

namespace stellar
{

class IOCOrderbookManager;

class TatonnementOracle {

	using int128_t = __int128;

	const IOCOrderbookManager& mOrderbookManager;

public:

	TatonnementOracle(const IOCOrderbookManager& orderbookManager);

	TatonnementOracle(const TatonnementOracle&) = delete;
	TatonnementOracle& operator=(const TatonnementOracle&) = delete;

	//map, not UnorderedMap, for deterministic ordering
	//caller's responsibility to initialize starting prices
	void computePrices(TatonnementControlParams const& params, std::map<Asset, uint64_t>& prices);
};

} /* stellar */