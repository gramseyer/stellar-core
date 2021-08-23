#pragma once

#include "util/UnorderedMap.h"
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

	UnorderedMap<Asset, size_t> mIndexMap;
	UnorderedMap<AssetPair, size_t, AssetPairHash> mAssetPairToRowMap;
	size_t indexPairToVarIndex(size_t sell, size_t buy) const;
	size_t assetPairToVarIndex(AssetPair assetPair) const;

	size_t assetToVarIndex(Asset asset) const;

	size_t numVars() const;

	std::optional<size_t>
	getNextPivotIndex() const;

	size_t
	getNextPivotConstraint(size_t pivotColumn) const;

	bool doPivot();

	void
	addRowToRow(size_t rowToAddIdx, size_t rowToBeAddedToIdx, int8_t coefficient);
	
	void
	multiplyRow(size_t rowIdx, int8_t coefficient);

public:

	TradeMaximizingSolver(std::vector<Asset> assets);

	void setUpperBound(AssetPair assetPair, int128_t upperBound);

	void doSolve();

};

} /* stellar */