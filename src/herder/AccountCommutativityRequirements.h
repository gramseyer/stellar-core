#pragma once

#include "util/UnorderedMap.h"
#include "xdr/Stellar-types.h"
#include "xdr/Stellar-transaction.h"
#include "xdr/Stellar-ledger-entries.h"

#include "util/XDROperators.h"

#include "ledger/LedgerHashUtils.h"

namespace stellar
{

class AbstractLedgerTxn;
class LedgerTxnHeader;


class AccountCommutativityRequirements {

	AccountID mSourceAccount;

	//nullopt if somebody overflows
	using AssetMap = UnorderedMap<Asset, std::optional<int64_t>>;

	AssetMap mRequiredAssets;

	bool checkCanAddAssetRequirement(
		AbstractLedgerTxn& ltx, Asset const& asset, int64_t amount);

	std::optional<int64_t>& getRequirement(Asset const& asset);


public:

	AccountCommutativityRequirements(AccountID source) : mSourceAccount(source) {}

	AccountCommutativityRequirements(AccountCommutativityRequirements const& other) = delete;
	AccountCommutativityRequirements& operator=(AccountCommutativityRequirements const& other) = delete;

	AccountCommutativityRequirements(AccountCommutativityRequirements&& other) = default;
	AccountCommutativityRequirements& operator=(AccountCommutativityRequirements&& other) = default;

	bool checkTrustLine(AbstractLedgerTxn& ltx, Asset asset) const;

	//implicitly checks trustline
	bool tryAddAssetRequirement(AbstractLedgerTxn& ltx, Asset asset, int64_t amount);

	//int64_t getNativeAssetReqs();

	void addAssetRequirement(Asset asset, std::optional<int64_t> amount);

	bool checkAvailableBalanceSufficesForNewRequirement(LedgerTxnHeader& header, AbstractLedgerTxn& ltx, Asset asset, int64_t amount);

	AssetMap const& getRequiredAssets() const {
		return mRequiredAssets;
	}

	void cleanZeroedEntries();

	bool isEmpty()
	{
		return mRequiredAssets.empty();
	}

	bool checkAccountHasSufficientBalance(AbstractLedgerTxn& ltx, LedgerTxnHeader& header);

};


} /* namespace stellar */