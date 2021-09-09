#pragma once

#include "lib/catch.hpp"

#include "xdr/Stellar-ledger-entries.h"

#include <cstdint>

namespace stellar
{

class AbstractLedgerTxn;

namespace txtest
{

std::vector<Asset> makeAssets(size_t numAssets);

void createLiquidityPool(
	Asset const& sell, 
	Asset const& buy, 
	int64_t sellAmount, 
	int64_t buyAmount, 
	AbstractLedgerTxn& ltx, 
	int32_t fee = LIQUIDITY_POOL_FEE_V18);

void addOffer(AbstractLedgerTxn& ltx, int32_t p_n, int32_t p_d, int64_t amount, Asset const& sell, Asset const& buy, uint64_t idx);

} /* txtest */

} /* stellar */
