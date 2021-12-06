#include "speedex/LiquidityPoolFrameBase.h"

#include "ledger/AssetPair.h"
#include "ledger/LedgerTxn.h"

#include "transactions/TransactionUtils.h"

#include "speedex/sim_utils.h"

namespace stellar {



LiquidityPoolFrameBaseLtx::LiquidityPoolFrameBaseLtx(AbstractLedgerTxn& ltx, AssetPair const& assetPair)
	: LiquidityPoolFrameBase()
	, mEntry(loadLiquidityPool(ltx, getPoolID(assetPair.selling, assetPair.buying)))
{}
	
LiquidityPoolFrameBaseLtx::operator bool() const {
	return (bool)mEntry;
}

std::pair<int64_t, int64_t>
LiquidityPoolFrameBaseLtx::getReserves() const {
	return {
		mEntry.current().data.liquidityPool().body.constantProduct().reserveA,
		mEntry.current().data.liquidityPool().body.constantProduct().reserveB
	};
}

uint32_t
LiquidityPoolFrameBaseLtx::getFee() const {
	return mEntry.current().data.liquidityPool().body.constantProduct().params.fee;
}

PoolID
LiquidityPoolFrameBaseLtx::poolID() const {
	return mEntry.current().data.liquidityPool().liquidityPoolID;
}

void
LiquidityPoolFrameBaseLtx::transfer(int64_t amountA, int64_t amountB) {
	mEntry.current().data.liquidityPool().body.constantProduct().reserveA += amountA;
	mEntry.current().data.liquidityPool().body.constantProduct().reserveB += amountB;
}


LiquidityPoolFrameBaseSim::LiquidityPoolFrameBaseSim(AMMConfig const& ammConfig)
	: LiquidityPoolFrameBase()
	, ammConfig(ammConfig)
	{}

LiquidityPoolFrameBaseSim::operator bool() const {
	return true;
}

std::pair<int64_t, int64_t>
LiquidityPoolFrameBaseSim::getReserves() const {
	return {
		ammConfig.amountA,
		ammConfig.amountB
	};
}

uint32_t
LiquidityPoolFrameBaseSim::getFee() const {
	return 30;
}

PoolID
LiquidityPoolFrameBaseSim::poolID() const {
	return getPoolID(makeSimAsset(ammConfig.assetA), makeSimAsset(ammConfig.assetB));
}

void
LiquidityPoolFrameBaseSim::transfer(int64_t amountA, int64_t amountB) {
	ammConfig.amountA += amountA;
	ammConfig.amountB += amountB;
}


} /* stellar */
