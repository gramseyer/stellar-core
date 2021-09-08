#include "herder/AccountCommutativityRequirements.h"
#include "transactions/TransactionUtils.h"
#include "ledger/TrustLineWrapper.h"
#include "util/types.h"

namespace stellar
{

// TODO cache balances and trustline checks?

// is a+b within limits
bool
satisfyNumericLimits(int64_t a, int64_t b)
{
	if (a < 0 && b < 0 && INT64_MIN - a > b)
	{
		return false;
	}
	if (a > 0 && b > 0 && INT64_MAX - a < b)
	{
		return false;
	}
	return true;
}

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

std::optional<int64_t>&
AccountCommutativityRequirements::getRequirement(Asset const& asset)
{
	auto iter = mRequiredAssets.find(asset);
	if (iter == mRequiredAssets.end()) {
		mRequiredAssets.emplace(asset, 0);
		iter = mRequiredAssets.find(asset);
	}
	return iter->second;
}

bool 
AccountCommutativityRequirements::checkCanAddAssetRequirement(
	AbstractLedgerTxn& ltx, Asset const& asset, int64_t amount) 
{

	if (!checkTrustLine(ltx, asset)) {
		return false;
	}

	if (!isCommutativeTxEnabledAsset(ltx, asset)) {
		return false;
	}

	auto const& currentRequirement = getRequirement(asset);
	if (!currentRequirement) {
		return false;
	}

	if (!satisfyNumericLimits(amount, *currentRequirement)) {
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

	auto& currentRequirement = *getRequirement(asset);

	currentRequirement += amount;
	return true;
}

void
AccountCommutativityRequirements::addAssetRequirement(
	Asset asset, std::optional<int64_t> amount)
{
	if (mRequiredAssets.find(asset) == mRequiredAssets.end()) {
		mRequiredAssets.emplace(asset, 0);
	}

	if (!amount) {
		mRequiredAssets[asset] = std::nullopt;
		return;
	}
	auto& requirement = getRequirement(asset);
	if (!requirement) {
		return;
	}

	auto derefAmount = *amount;

	if (!satisfyNumericLimits(derefAmount, *requirement)) {
		requirement = std::nullopt;
		return;
	}
	if (requirement) {
		*requirement += derefAmount;
		return;
	}
}

bool
AccountCommutativityRequirements::checkAvailableBalanceSufficesForNewRequirement(
	LedgerTxnHeader& header, AbstractLedgerTxn& ltx, Asset asset, int64_t amount)
{
	if (!checkCanAddAssetRequirement(ltx, asset, amount)) {
		std::printf("checkCanAddAssetRequirement fail\n");
		return false;
	}
	auto& currentRequirement = getRequirement(asset);

	auto currentBalance = getAvailableBalance(header, ltx, mSourceAccount, asset);


	if (!currentRequirement) {
		return false;
	}

	std::printf("curBal %lld curReq %lld amount %lld", currentBalance, *currentRequirement, amount);

	if (amount + *currentRequirement <= currentBalance) {
		std::printf("good result, return true\n");
		return true;
	}
	std::printf("currentBalance was too small\n");
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
		} 
		else
		{
			iter++;
		}
	}
}

bool 
AccountCommutativityRequirements::checkAccountHasSufficientBalance(AbstractLedgerTxn& ltx, LedgerTxnHeader& header) {
	for (auto const& [asset, amount] : mRequiredAssets) {
		if (!isCommutativeTxEnabledAsset(ltx, asset)) {
			return false;
		}
		if (!checkTrustLine(ltx, asset)) {
			return false;
		}
		if (!amount) {
			return false;
		}
		std::printf("requirement: %ld currentBalance %ld\n", *amount, getAvailableBalance(header, ltx, mSourceAccount, asset));

		if (*amount > getAvailableBalance(header, ltx, mSourceAccount, asset)) {

			return false;
		}
	}
	return true;
}

} /* stellar */