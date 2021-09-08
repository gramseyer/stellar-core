#include "speedex/IOCOrderbookManager.h"

#include "ledger/LedgerTxn.h"
#include "speedex/OrderbookClearingTarget.h"
#include "speedex/LiquidityPoolFrame.h"

#include "transactions/TransactionUtils.h"

#include "util/types.h"

#include "speedex/DemandUtils.h"

namespace stellar {

void 
IOCOrderbookManager::throwIfSealed() const {
	if (mSealed) {
		throw std::runtime_error("op on sealed orderbook manager");
	}
}
void 
IOCOrderbookManager::throwIfNotSealed() const {
	if (!mSealed) {
		throw std::runtime_error("op on non sealed orderbook manager");
	}
}

void
IOCOrderbookManager::throwIfAlreadyCleared() const
{
	if (mCleared)
	{
		throw std::runtime_error("manager has already been cleared! (must do clearing on base layer ledgertxn)");
	}
}

void
IOCOrderbookManager::doPriceComputationPreprocessing() {
	for (auto & [_, orderbook] : mOrderbooks)
	{
		orderbook.doPriceComputationPreprocessing();
	}
}


IOCOrderbook&
IOCOrderbookManager::getOrCreateOrderbook(AssetPair const& assetPair) {
	return (mOrderbooks.emplace(assetPair, assetPair).first->second);
}

size_t
IOCOrderbookManager::numOpenOrderbooks() const {
	return mOrderbooks.size();
}

void
IOCOrderbookManager::commitChild(const IOCOrderbookManager& child) {
	
	if (numOpenOrderbooks() > 0)
	{
		child.throwIfAlreadyCleared();
	}

	if (child.numOpenOrderbooks() > 0) {
		throwIfSealed();
		for (const auto& orderbook : child.mOrderbooks) {
			getOrCreateOrderbook(orderbook.first).commitChild(orderbook.second);
		}
	}
}

void
IOCOrderbookManager::addOffer(AssetPair const& assetPair, IOCOffer const& offer) {
	throwIfSealed();
	getOrCreateOrderbook(assetPair).addOffer(offer);
}

void
IOCOrderbookManager::clear() { // no offer unwinding here b/c only called when ltx rollsback
	mOrderbooks.clear();
}


void 
IOCOrderbookManager::clearOrderbook(AbstractLedgerTxn& ltx, OrderbookClearingTarget& target) {
	auto assetPair = target.getAssetPair();
	auto baseFrame = BaseLiquidityPoolFrame(ltx, assetPair);
	auto lpFrame = LiquidityPoolFrame(baseFrame, assetPair);
	getOrCreateOrderbook(assetPair).clearOffers(ltx, target, lpFrame);
}

void
IOCOrderbookManager::sealBatch() {
	throwIfSealed();
	mSealed = true;
	doPriceComputationPreprocessing();
}

void IOCOrderbookManager::returnToSource(AbstractLedgerTxn& ltx, Asset asset, int64_t amount) {
	if (asset.type() == ASSET_TYPE_NATIVE) {
		auto header = ltx.loadHeader();
		header.current().feePool += amount;
	} else if (asset.type() == ASSET_TYPE_CREDIT_ALPHANUM4 || asset.type() == ASSET_TYPE_CREDIT_ALPHANUM12) {
		auto issuerAccountID = getIssuer(asset);

		auto issuerAccount = loadAccount(ltx, issuerAccountID);

		if (!issuerAccount) {
			throw std::runtime_error("speedex assets can't delete issuer account");
		}

		auto header = ltx.loadHeader();

		if (!addBalance(header, issuerAccount, amount)) {
			throw std::runtime_error("failed to add balance for unknown reason");
		}
	} else {
		throw std::runtime_error("invalid asset type: can't trade lp shares (unimpl)");
	}
}

void 
IOCOrderbookManager::clearBatch(AbstractLedgerTxn& ltx, const BatchSolution& solution) {
	throwIfNotSealed();
	throwIfAlreadyCleared();

	auto orderbookTargets = solution.produceClearingTargets();

	for (auto& target : orderbookTargets) {
		clearOrderbook(ltx, target);
	}
	UnorderedMap<Asset, int64_t> roundingErrors;

	for (auto& [_, orderbook] : mOrderbooks) {
		orderbook.finish(ltx);
	}
	for (auto& target : orderbookTargets) {
		auto assetPair = target.getAssetPair();
		roundingErrors[assetPair.selling] += target.getRealizedSellAmount();
		roundingErrors[assetPair.buying] -= target.getRealizedBuyAmount();
	}

	for (auto& [asset, roundingError] : roundingErrors) {
		if (roundingError < 0) {
			throw std::runtime_error("market paid out more than it received!");
		}
		returnToSource(ltx, asset, roundingError);
	}
	mOrderbooks.clear();
	mCleared = true;
}

void 
IOCOrderbookManager::demandQuery(
	std::map<Asset, uint64_t> const& prices, 
	SupplyDemand& supplyDemand,
	uint8_t smoothMult) const
{
	for (auto const& [assetPair, orderbook] : mOrderbooks)
	{
		auto sellPrice = prices.at(assetPair.selling);
		auto buyPrice = prices.at(assetPair.buying);


		auto tradeAmount = orderbook.cumulativeOfferedForSaleTimesPrice(sellPrice, buyPrice, smoothMult);

		supplyDemand.addSupplyDemand(assetPair, tradeAmount);
	}
}

IOCOrderbookManager::int128_t 
IOCOrderbookManager::demandQueryOneAssetPair(AssetPair const& tradingPair, std::map<Asset, uint64_t> const& prices) const
{
	auto iter = mOrderbooks.find(tradingPair);
	if (iter == mOrderbooks.end()) {
		return 0;
	}
	return iter->second.cumulativeOfferedForSaleTimesPrice(prices.at(tradingPair.selling), prices.at(tradingPair.buying), 0);
}

} /* stellar */
