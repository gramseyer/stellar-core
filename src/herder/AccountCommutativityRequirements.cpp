#include "herder/AccountCommutativityRequirements.h"
#include "transactions/TransactionUtils.h"
#include "ledger/TrustLineWrapper.h"

namespace stellar
{

// TODO cache balances and trustline checks?

bool
AccountCommutativityRequirements::checkTrustLine(
	AbstractLedgerTxn& ltx, Asset asset) const
{
	if (asset.type() == ASSET_TYPE_NATIVE)
	{
		return true;
	}

	auto tl = loadTrustLine(ltx, mSourceAccount, asset);

	if (!tl) {
		return false;
	}

	return tl.isCommutativeTxEnabledTrustLine();
}

bool 
AccountCommutativityRequirements::checkCanAddAssetRequirement(
	AbstractLedgerTxn& ltx, Asset asset, int64_t amount) 
{

	if (amount < 0) {
		return false;
	}

	if (!checkTrustLine(ltx, asset)) {
		return false;
	}

	if (!isCommutativeTxEnabledAsset(ltx, asset)) {
		return false;
	}

	auto const& currentRequirement = mRequiredAssets[asset];

	if (INT64_MAX - currentRequirement < amount) {
		return false;
	}
	return true;
}


bool
AccountCommutativityRequirements::tryAddAssetRequirement(
	AbstractLedgerTxn& ltx, Asset asset, int64_t amount)
{
	if (!checkCanAddAssetRequirement(ltx, asset, amount)) {
		return false;
	}

	auto& currentRequirement = mRequiredAssets[asset];

	currentRequirement += amount;
	return true;
}
void
AccountCommutativityRequirements::addAssetRequirement(
	Asset asset, int64_t amount)
{
	mRequiredAssets[asset] += amount;
}

bool
AccountCommutativityRequirements::checkAvailableBalanceSufficesForNewRequirement(
	LedgerTxnHeader& header, AbstractLedgerTxn& ltx, Asset asset, int64_t amount)
{
	if (!checkCanAddAssetRequirement(ltx, asset, amount)) {
		return false;
	}
	auto& currentRequirement = mRequiredAssets[asset];

	auto currentBalance = getAvailableBalance(header, ltx, mSourceAccount, asset);

	if (amount + currentRequirement <= currentBalance) {
		return true;
	}
	return false;
}

void
AccountCommutativityRequirements::cleanZeroedEntries()
{
	for (auto iter = mRequiredAssets.begin(); iter != mRequiredAssets.end();)
	{
		if (iter -> second == 0)
		{
			iter = mRequiredAssets.erase(iter);
		} else
		{
			iter++;
		}
	}
}

bool 
AccountCommutativityRequirements::checkAccountHasSufficientBalance(AbstractLedgerTxn& ltx, LedgerTxnHeader& header) {
	for (auto const& [asset, amount] : mRequiredAssets) {
		if (!isCommutativeTxEnabledAsset(ltx, asset)) {
			//std::printf("comm enabled asset fail\n");
			return false;
		}
		if (!checkTrustLine(ltx, asset)) {
			//std::printf("trustline fail\n");
			return false;
		}
		if (amount > getAvailableBalance(header, ltx, mSourceAccount, asset)) {
			//std::printf("requirement: %ld currentBalance %ld\n", amount, getAvailableBalance(header, ltx, mSourceAccount, asset));

			return false;
		}
	}
	return true;
}





} /* stellar */