#pragma once
#include "speedex/TatonnementControls.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include "util/XDROperators.h"

#include "ledger/LedgerHashUtils.h"

#include  <cstdint>

#include "speedex/DemandOracle.h"

namespace stellar
{

class IOCOrderbookManager;
class LiquidityPoolSetFrame;

class TatonnementOracle {

	using int128_t = __int128;

	DemandOracle mDemandOracle;

	void demandQuery(std::map<Asset, uint64_t> const& prices, std::map<Asset, int128_t>& demands, uint8_t taxRate, uint8_t smoothMult) const;

public:

	TatonnementOracle(IOCOrderbookManager const& orderbookManager, LiquidityPoolSetFrame const& liquidityPools);

	TatonnementOracle(const TatonnementOracle&) = delete;
	TatonnementOracle& operator=(const TatonnementOracle&) = delete;

	//map, not UnorderedMap, for deterministic ordering
	//caller's responsibility to initialize starting prices
	void computePrices(TatonnementControlParams const& params, std::map<Asset, uint64_t>& prices, uint32_t printFrequency = 0);
};

} /* stellar */