#pragma once

#include "ledger/LedgerTxnEntry.h"

#include "xdr/speedex-sim.h"

namespace stellar {


class LiquidityPoolFrameBase {

public:

	virtual 
	operator bool() const = 0;
	virtual
	std::pair<int64_t, int64_t> getReserves() const = 0;
	virtual
	uint32_t getFee() const = 0;
	virtual
	PoolID poolID() const = 0;

	virtual
	void transfer(int64_t amountA, int64_t amountB) = 0;

	virtual
	~LiquidityPoolFrameBase() {};
};


class AbstractLedgerTxn;
struct AssetPair;

class LiquidityPoolFrameBaseLtx : public LiquidityPoolFrameBase {

	LedgerTxnEntry mEntry;

public:

	LiquidityPoolFrameBaseLtx(AbstractLedgerTxn& ltx, AssetPair const& assetPair);
	
	operator bool() const override final;
	std::pair<int64_t, int64_t> getReserves() const override final;
	uint32_t getFee() const override final;
	PoolID poolID() const override final;

	void transfer(int64_t amountA, int64_t amountB) override final;

	~LiquidityPoolFrameBaseLtx() override final = default;
};

class LiquidityPoolFrameBaseSim : public LiquidityPoolFrameBase {

	AMMConfig ammConfig;

public:

	LiquidityPoolFrameBaseSim(AMMConfig const& ammConfig);

	operator bool() const override final;
	std::pair<int64_t, int64_t> getReserves() const override final;
	uint32_t getFee() const override final;
	PoolID poolID() const override final;

	void transfer(int64_t amountA, int64_t amountB) override final;

	~LiquidityPoolFrameBaseSim() override final = default;

};

} /* stellar */

