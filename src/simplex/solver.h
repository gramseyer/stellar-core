#pragma once

#include "util/UnorderedMap.h"
#include "util/UnorderedSet.h"
#include "util/XDROperators.h"
#include "ledger/AssetPair.h"

#include "ledger/LedgerHashUtils.h"

#include <cstdint>
#include <vector>



namespace stellar {


class TradeMaximizingSolver {

	size_t mNumAssets;

	using int128_t = __int128_t;
	using Row = std::pair<std::vector<int8_t>, int128_t>;

	std::vector<Row> mCoefficients;

	using row_idx_t = size_t;
	using col_idx_t = size_t;

	UnorderedMap<Asset, size_t> mIndexMap; // asset to number

	UnorderedMap<AssetPair, row_idx_t, AssetPairHash> mAssetPairToRowMap;

	std::vector<bool> mActiveYijs;

	UnorderedMap<std::pair<size_t, size_t>, int128_t> mSolutionMap;

	UnorderedMap<row_idx_t, col_idx_t> mActiveBasis;

	bool mSolved;

	void throwIfSolved() const;
	void throwIfUnsolved() const;

	size_t indexPairToVarIndex(size_t sell, size_t buy) const;
	//returns nullopt if not yij index
	std::optional<std::pair<size_t, size_t>> varIndexToIndexPair(size_t yijIdx) const;

	size_t assetPairToVarIndex(AssetPair assetPair) const;

	size_t assetToVarIndex(Asset asset) const;

	size_t numVars() const;

	size_t numYijEijVars() const;
	size_t numYijVars() const;

	std::optional<size_t>
	getNextPivotIndex() const;

	size_t
	getNextPivotConstraint(size_t pivotColumn) const;

	bool doPivot();

	void
	addRowToRow(size_t rowToAddIdx, size_t rowToBeAddedToIdx, int8_t coefficient);
	
	void
	multiplyRow(size_t rowIdx, int8_t coefficient);

	bool isBasisCol(size_t colIdx) const;

	void constructSolution();

	void printRow(size_t idx) const;
	void printTableau() const;

public:

	TradeMaximizingSolver(std::vector<Asset> assets);

	TradeMaximizingSolver(const TradeMaximizingSolver&) = delete;
	TradeMaximizingSolver& operator=(const TradeMaximizingSolver&) = delete;

	void setUpperBound(AssetPair const& assetPair, int128_t upperBound);

	void doSolve();

	int128_t getRowResult(AssetPair const& assetPair) const;

	UnorderedMap<AssetPair, int128_t, AssetPairHash> getSolution() const;

	void printSolution() const;

};

} /* stellar */