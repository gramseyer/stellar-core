#include "lib/catch.hpp"
#include "speedex/TatonnementControls.h"

#include "speedex/DemandUtils.h"

#include "test/TxTests.h"

#include "test/TestUtils.h"
#include "test/test.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include <map>

using namespace stellar::txtest;
using namespace stellar;

static std::vector<Asset> makeAssets(size_t numAssets)
{
	auto acct = getAccount("asdf");

	std::vector<Asset> out;

	for (auto i = 0u; i < numAssets; i++)
	{
		out.push_back(makeAsset(acct, fmt::format("A{}", i)));
	}

	return out;
}


TEST_CASE("objective function", "[speedex]")
{
	using int128_t = __int128;

	SupplyDemand demands1, demands2;

	auto assets = makeAssets(10);

	demands1.mSupplyDemand[assets[0]] = {10000, 0};
	demands2.mSupplyDemand[assets[0]] = {10001, 0};

	TatonnementObjectiveFn obj1 = demands1.getObjective(), obj2 = demands2.getObjective();

	REQUIRE(obj1.isBetterThan(obj2, 0, 1));
	REQUIRE(!obj2.isBetterThan(obj1, 0, 1));

	REQUIRE(obj1.isBetterThan(obj2, 100, 101));
	REQUIRE(obj2.isBetterThan(obj1, 100, 101));
}

TEST_CASE("step adjustments", "[speedex]")
{

	auto check = [] (uint8_t radix) {
		TatonnementControlParams params;
		params.mStepSizeRadix = radix;

		//minimum possible step size adjustments
		params.mStepUp = ((1u) << radix) + 1;
		params.mStepDown = ((1u) << radix) - 1;

		TatonnementControlParamsWrapper wrapper(params);
		REQUIRE(wrapper.stepUp(wrapper.kMinStepSize) > wrapper.kMinStepSize);
		REQUIRE(wrapper.stepDown(wrapper.kMinStepSize) < wrapper.kMinStepSize);	
	};

	for (uint8_t i = 1; i < 8; i++) {
		check(i);
	}
}

TEST_CASE("price adjust", "[speedex]")
{
	using int128_t = __int128;

	TatonnementControlParams params;
	params.mStepRadix = 80;

	TatonnementControlParamsWrapper wrapper(params);

	int128_t demand = ((int128_t)1) << 80;

	uint64_t startingPrice = 1;
	REQUIRE(wrapper.setTrialPrice(startingPrice, demand, 1) == 2);

	REQUIRE(wrapper.setTrialPrice(startingPrice, demand, 10) == 11);

	REQUIRE(wrapper.setTrialPrice(startingPrice, demand, 0) > 0);
}
