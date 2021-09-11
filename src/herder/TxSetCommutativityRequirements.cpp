#include "herder/TxSetCommutativityRequirements.h"

#include "ledger/LedgerTxnHeader.h"
#include "ledger/LedgerTxn.h"
#include "util/XDROperators.h"

#include "ledger/LedgerHashUtils.h"


#include "transactions/TransactionUtils.h"


namespace stellar {


AccountCommutativityRequirements& 
TxSetCommutativityRequirements::getRequirements(AccountID account)
{
	auto [iter, _] = mAccountRequirements.emplace(std::make_pair(account, account));
	return iter -> second;
}

bool 
TxSetCommutativityRequirements::canAddFee(LedgerTxnHeader& header, AbstractLedgerTxn& ltx, AccountID feeAccount, int64_t fee) 
{

	//Possibly redundant check
	{
		auto accountEntry = loadAccount(ltx, feeAccount);
		if (!accountEntry)
		{
			return false;
		}
	}

	auto& reqs = getRequirements(feeAccount);
	if (!reqs.checkAvailableBalanceSufficesForNewRequirement(header, ltx, getNativeAsset(), fee))
	{
		return false;
	}
	return true;
}

void 
TxSetCommutativityRequirements::addFee(AccountID feeAccount, int64_t fee) 
{
	getRequirements(feeAccount).addAssetRequirement(getNativeAsset(), fee);
}

bool 
TxSetCommutativityRequirements::tryAddTransaction(TransactionFrameBasePtr tx, AbstractLedgerTxn& ltx)
{

	LedgerTxnHeader header = ltx.loadHeader();

	auto reqs = tx->getCommutativityRequirements(ltx);
	if (!reqs) {
		return false;
	}

	for (auto const& [acct, acctReqs] : reqs->getRequirements()) {
		auto& prevAcctReqs = getRequirements(acct);

		for (auto& req : acctReqs.getRequiredAssets()) {
			if (!req.second) {
				return false;
			}
			if (!prevAcctReqs.checkAvailableBalanceSufficesForNewRequirement(header, ltx, req.first, *req.second))
			{
				return false;
			}
		}
	}

	for (auto const& [acct, acctReqs] : reqs -> getRequirements()) {
		auto& prevAcctReqs = getRequirements(acct);

		for (auto& req : acctReqs.getRequiredAssets()) {
			prevAcctReqs.addAssetRequirement(req.first, *req.second);
		}
	}

	return true;
}

//Just checks that commutative txs actually can compute asset requirements
bool
TxSetCommutativityRequirements::validateAndAddTransaction(TransactionFrameBasePtr tx, AbstractLedgerTxn& ltx) {
	AccountID sourceAccount = tx->getSourceID();

	auto reqs = tx->getCommutativityRequirements(ltx);

	if (!reqs) {
		return false;
	}

	for (auto const& [acct, acctReqs] : reqs -> getRequirements()) {
		auto& prevAcctReqs = getRequirements(acct);
		for (auto const& req : acctReqs.getRequiredAssets()) {
			prevAcctReqs.addAssetRequirement(req.first, req.second);
		}
	}

	return true;
}

bool 
TxSetCommutativityRequirements::tryReplaceTransaction(TransactionFrameBasePtr newTx, 
													  TransactionFrameBasePtr oldTx, 
													  AbstractLedgerTxn& ltx)
{

	AccountID sourceAccount = newTx->getSourceID();
	if (!(oldTx->getSourceID() == sourceAccount)) {
		throw std::logic_error("can't replace from different account");
	}

	auto newReqs = newTx -> getCommutativityRequirements(ltx);
	auto oldReqs = oldTx -> getCommutativityRequirements(ltx);

	if (!newReqs) {
		// newTx failed some validation check
		return false;
	}
	if (!oldReqs) {
		throw std::runtime_error("bad tx got into queue!");
	}

	LedgerTxnHeader header = ltx.loadHeader();

	// newreqs includes negative oldReqs, which includes the old fee requirement
	for (auto const& [acct, oldAcctReqs] : oldReqs -> getRequirements())
	{
		for (auto const& [asset, amount] : oldAcctReqs.getRequiredAssets())
		{
			//oldAcctReqs should be always valid, so never nullopt
			newReqs->addAssetRequirement(acct, asset, -*amount);
		}
	}

	for (auto const& [acct, newAcctReqs] : newReqs -> getRequirements())
	{
		auto& prevAcctReqs = getRequirements(acct);
		for (auto const& [asset, amount] : newAcctReqs.getRequiredAssets()) {
			if (!amount) {
				// overflow on a requirement
				return false;
			}
			if (!prevAcctReqs.checkAvailableBalanceSufficesForNewRequirement(header, ltx, asset, *amount)) {
				// insufficient balance
				return false;
			}
		}

	}

	for (auto const& [acct, newAcctReq] : newReqs->getRequirements())
	{
		auto& prevReqs = getRequirements(acct);
		for (auto const& [asset, amount] : newAcctReq.getRequiredAssets()) {
			prevReqs.addAssetRequirement(asset, amount);
		}
	}

	//if (!(newTx -> getFeeSourceID() == oldTx -> getFeeSourceID()))
	//{
	//	addFee(oldTx -> getFeeSourceID(), -oldTx -> getFeeBid());
	//}
	return true;
}

bool 
TxSetCommutativityRequirements::tryCleanAccountEntry(AccountID account)
{
	auto iter = mAccountRequirements.find(account);
	if (iter == mAccountRequirements.end())
	{
		return false;
	}
	iter -> second.cleanZeroedEntries();

	if (iter -> second.isEmpty())
	{
		mAccountRequirements.erase(iter);
		return true;
	}
	return false;

}

bool 
TxSetCommutativityRequirements::checkAccountHasSufficientBalance(AccountID account, AbstractLedgerTxn& ltx, LedgerTxnHeader& header) {
	return getRequirements(account).checkAccountHasSufficientBalance(ltx, header);
}

#ifdef BUILD_TESTS
std::optional<int64_t> 
TxSetCommutativityRequirements::getReq(AccountID account, Asset asset)
{
	return getRequirements(account).getRequirement(asset);
}
#endif

} /* stellar */
