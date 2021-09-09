#include "speedex/test/TatonnementTestUtils.h"


#include "ledger/LedgerTxn.h"

#include "speedex/IOCOffer.h"


#include "test/TxTests.h"
#include "test/TestUtils.h"
#include "test/test.h"

#include "transactions/TransactionUtils.h"

namespace stellar
{

namespace txtest
{

std::vector<Asset> makeAssets(size_t numAssets, TestAccount& issuer)
{
	std::vector<Asset> out;

	for (auto i = 0u; i < numAssets; i++)
	{
		out.push_back(issuer.asset(fmt::format("A{}", i)));
	}

	return out;
}

std::vector<Asset> makeAssets(size_t numAssets)
{
	auto acct = getAccount("asdf");

	std::vector<Asset> out;

	for (auto i = 0u; i < numAssets; i++)
	{
		out.push_back(makeAsset(acct, fmt::format("A{}", i)));
	}

	return out;
}

PoolID createLiquidityPool(Asset const& sell, Asset const& buy, int64_t sellAmount, int64_t buyAmount, AbstractLedgerTxn& ltx, int32_t fee)
{
	PoolID id = getPoolID(sell, buy);

	LedgerEntry lpEntry;
    lpEntry.data.type(LIQUIDITY_POOL);
    auto& lp = lpEntry.data.liquidityPool();
    lp.liquidityPoolID = id;
    lp.body.type(LIQUIDITY_POOL_CONSTANT_PRODUCT);

    auto &constProd = lp.body.constantProduct();


    auto& params = constProd.params;
    params.fee = fee;

  	if (sell < buy) {
  		params.assetA = sell;
  		params.assetB = buy;

  		constProd.reserveA = sellAmount;
  		constProd.reserveB = buyAmount;
  	} 
  	else
  	{
  		params.assetA = buy;
  		params.assetB = sell;

  		constProd.reserveA = buyAmount;
  		constProd.reserveB = sellAmount;
  	}

    ltx.create(lpEntry);
    return id;
}

TestAccount getIssuanceLimitedAccount(TestAccount& rootAccount, std::string name, int64_t balance)
{
	auto out = rootAccount.create(name, balance);
	out.setAssetIssuanceLimited();
	return out;
}

void setNonIssuerTrustlines(TestAccount& acct, std::vector<Asset> const& assets)
{
	for (auto const& asset : assets)
	{
		acct.changeTrust(asset, INT64_MAX);
	}
}

void fundTrader(TestAccount& trader, TestAccount& issuer, std::vector<Asset> const& assets, int64_t amount)
{
	for (auto const& asset : assets)
	{
		issuer.pay(trader, asset, amount);
	}
}


void 
setSpeedexAssets(AbstractLedgerTxn& ltx, std::vector<Asset> const& assets) {
	auto sc = loadSpeedexConfig(ltx);

	auto& scAssets = sc.current().data.speedexConfig().speedexAssets;

	for (auto const& asset : assets)
	{
		scAssets.push_back(asset);
	}
}

void addOffer(AbstractLedgerTxn& ltx, AccountID acct, int32_t p_n, int32_t p_d, int64_t amount, Asset const& sell, Asset const& buy, uint64_t idx)
{
	Price p;
	p.n = p_n;
	p.d = p_d;

	//AccountID acct = getAccount("blah").getPublicKey();

	IOCOffer offer(amount, p, acct, idx, 0);

	AssetPair tradingPair {
		.selling = sell,
		.buying = buy
	};
	ltx.addSpeedexIOCOffer(tradingPair, offer);
}

std::optional<SpeedexOfferClearingStatus> 
findOfferClearingStatus(SpeedexResults const& res, AccountID srcAccount, uint64_t seqNum, uint32_t offerIdx)
{
	for (auto const& offerRes : res.offerStatuses)
	{
		if (offerRes.sourceAccount == srcAccount && offerRes.seqNum == seqNum && offerRes.offerIndex == offerIdx)
		{
			return offerRes;
		}
	}
	return std::nullopt;
}

std::optional<SpeedexLiquidityPoolClearingStatus>
findLPClearingStatus(SpeedexResults const& res, PoolID pool)
{
	for (auto const& lpRes : res.lpStatuses)
	{
		if (lpRes.pool == pool) {
			return lpRes;
		}
	}
	return std::nullopt;
}


} /* txtest */
} /* stellar */
