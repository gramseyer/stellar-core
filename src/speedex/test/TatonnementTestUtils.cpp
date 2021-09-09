#include "speedex/test/TatonnementTestUtils.h"


#include "ledger/LedgerTxn.h"

#include "speedex/IOCOffer.h"

namespace stellar
{

namespace txtest
{


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

void createLiquidityPool(Asset const& sell, Asset const& buy, int64_t sellAmount, int64_t buyAmount, AbstractLedgerTxn& ltx, int32_t fee)
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
}

void addOffer(AbstractLedgerTxn& ltx, int32_t p_n, int32_t p_d, int64_t amount, Asset const& sell, Asset const& buy, uint64_t idx)
{
	Price p;
	p.n = p_n;
	p.d = p_d;

	AccountID acct = getAccount("blah").getPublicKey();

	IOCOffer offer(amount, p, acct, idx, 0);

	AssetPair tradingPair {
		.selling = sell,
		.buying = buy
	};
	ltx.addSpeedexIOCOffer(tradingPair, offer);
}

} /* txtest */
} /* stellar */
