#include "speedex/LiquidityPoolFrame.h"

#include "ledger/AssetPair.h"
#include "ledger/LedgerTxn.h"

#include "transactions/TransactionUtils.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace stellar {

LiquidityPoolFrame::LiquidityPoolFrame(AbstractLedgerTxn& ltx, AssetPair const& assetPair)
	: mTradingPair(assetPair)
{
	auto poolID = getPoolID(assetPair.selling, assetPair.buying);
	mEntry = loadLiquidityPool(ltx, poolID) ;
}

LiquidityPoolFrame::operator bool() const {
	return (bool) mEntry;
}

std::pair<int64_t, int64_t> 
LiquidityPoolFrame::getSellBuyAmounts() const {
	if (!mEntry) {
		return std::make_pair<int64_t, int64_t>(0, 0);
	}

	int64_t reserveA = mEntry.current().data.liquidityPool().body.constantProduct().reserveA;
	int64_t reserveB = mEntry.current().data.liquidityPool().body.constantProduct().reserveB;

	if (mTradingPair.selling < mTradingPair.buying) {
		return {reserveA, reserveB};
	} else {
		return {reserveB, reserveA};
	}
}

double
LiquidityPoolFrame::getFeeRate() const {
	if (!mEntry) {
		return 0.0;
	}
	return (((double) mEntry.current().data.liquidityPool().body.constantProduct().params.fee) / (100.0));
}

double
LiquidityPoolFrame::getMinPriceRatio() const {
	auto [sellAmount, buyAmount] = getSellBuyAmounts();
	double feeRate = getFeeRate();
	return (((double) buyAmount) / ((double) sellAmount)) * (1.0 / (1.0 -  feeRate));
}

int64_t 
LiquidityPoolFrame::amountOfferedForSale(uint64_t sellPrice, uint64_t buyPrice) const {
	//TODO run in fixed point
	double offeredRatio = ((double) sellPrice) / ((double) buyPrice);

	auto [sellAmount, buyAmount] = getSellBuyAmounts();

	if (sellAmount == 0) {
		return 0;
	}

	auto minRatio = getMinPriceRatio();

	if (offeredRatio < minRatio) {
		return 0;
	}

	int128_t k = ((int128_t) sellAmount) * ((int128_t) buyAmount);


	auto invSqrt = [] (double val) {
		return std::pow(val, -0.5);
	};

	auto sqrt = [] (double val) {
		return std::pow(val, 0.5);
	};

	return std::floor(sqrt((double)k) * (invSqrt(minRatio) - invSqrt(offeredRatio)));
}

void 
LiquidityPoolFrame::assertValidSellAmount(int64_t sellAmount, uint64_t sellPrice, uint64_t buyPrice) const {
	if (sellAmount > amountOfferedForSale(sellPrice, buyPrice)) {
		throw std::runtime_error("invalid sell amount!");
	}
	if (sellAmount < 0) {
		throw std::runtime_error("negative sell amount?");
	}
}

// asserts that constant product invariant is preserved
void
LiquidityPoolFrame::assertValidTrade(int64_t sellAmount, int64_t buyAmount, uint64_t sellPrice, uint64_t buyPrice) const {
	assertValidSellAmount(sellAmount, sellPrice, buyPrice);


	auto [oldSellAmount, oldBuyAmount] = getSellBuyAmounts();

	int128_t prevK = ((int128_t) oldSellAmount) * ((int128_t) oldBuyAmount);

	int128_t newK = ((int128_t) (oldSellAmount - sellAmount)) 
		* ((int128_t) (oldBuyAmount + std::floor(((double) buyAmount) * (1.0 - getFeeRate()))));

	if (newK < prevK) {
		throw std::runtime_error("constant product invariant not preserved");
	}
}

void 
LiquidityPoolFrame::doTransfer(int64_t sellAmount, int64_t buyAmount, uint64_t sellPrice, uint64_t buyPrice) {

	if (!mEntry) {
		throw std::runtime_error("can't modify nonexistent lp");
	}

	assertValidTrade(sellAmount, buyAmount, sellPrice, buyPrice);

	if (mTradingPair.selling < mTradingPair.buying) {
		mEntry.current().data.liquidityPool().body.constantProduct().reserveA -= sellAmount;
		mEntry.current().data.liquidityPool().body.constantProduct().reserveB += buyAmount;
	} else {
		mEntry.current().data.liquidityPool().body.constantProduct().reserveB -= sellAmount;
		mEntry.current().data.liquidityPool().body.constantProduct().reserveA += buyAmount;
	}
}




} /* stellar */