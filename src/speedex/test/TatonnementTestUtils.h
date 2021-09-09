#pragma once

#include "lib/catch.hpp"

#include "xdr/Stellar-ledger-entries.h"
#include "xdr/Stellar-ledger.h"


#include "test/TestAccount.h"

#include <cstdint>
#include <optional>

namespace stellar
{

class AbstractLedgerTxn;

namespace txtest
{


std::vector<Asset> makeAssets(size_t numAssets, TestAccount& issuer);

std::vector<Asset> makeAssets(size_t numAssets);

PoolID
createLiquidityPool(
	Asset const& sell, 
	Asset const& buy, 
	int64_t sellAmount, 
	int64_t buyAmount, 
	AbstractLedgerTxn& ltx, 
	int32_t fee = LIQUIDITY_POOL_FEE_V18);

void addOffer(AbstractLedgerTxn& ltx, AccountID acct, int32_t p_n, int32_t p_d, int64_t amount, Asset const& sell, Asset const& buy, uint64_t idx);

void setSpeedexAssets(AbstractLedgerTxn& ltx, std::vector<Asset> const& assets);

TestAccount 
getIssuanceLimitedAccount(TestAccount& rootAccount, std::string name, int64_t balance);
void setNonIssuerTrustlines(TestAccount& acct, std::vector<Asset> const& assets);

void fundTrader(TestAccount& trader, TestAccount& issuer, std::vector<Asset> const& assets, int64_t amount = 1000000);

std::optional<SpeedexOfferClearingStatus> 
findOfferClearingStatus(SpeedexResults const& res, AccountID srcAccount, uint64_t seqNum, uint32_t offerIdx);


std::optional<SpeedexLiquidityPoolClearingStatus>
findLPClearingStatus(SpeedexResults const& res, PoolID pool);
} /* txtest */

} /* stellar */
