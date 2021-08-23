#include "simplex/solver.h"

namespace stellar {


size_t
TradeMaximizingSolver::numVars() const {
	return (mNumAssets - 1) * (mNumAssets - 1) * 2;
}

size_t
TradeMaximizingSolver::indexPairToVarIndex(size_t sell, size_t buy) const {
	return sell * (mNumAssets - 1) + (buy > sell ? buy - 1 : buy);
}

size_t 
TradeMaximizingSolver::assetPairToVarIndex(AssetPair assetPair) const {
	auto sellIdx = mIndexMap.at(assetPair.selling);
	auto buyIdx = mIndexMap.at(assetPair.buying);
	return indexPairToVarIndex(sellIdx, buyIdx);
}


TradeMaximizingSolver::TradeMaximizingSolver(std::vector<Asset> assets) {
	size_t idx = 0;
	for (auto asset : assets) {
		mIndexMap[asset] = idx;
		idx++;
	}
	mNumAssets = idx;
	auto nVars = numVars();
	size_t numRows = mNumAssets + 1; // last one is objective
	mCoefficients.resize(numRows);
	for (auto& row : mCoefficients) {
		row.first.resize(nVars);
	}

	for (size_t i = 0; i < mNumAssets; i++) {
		auto& constraintRow = mCoefficients[i+1];
		for (size_t j = 0; j < mNumAssets; j++) {
			if (i != j) {
				auto yijIdx = indexPairToVarIndex(i, j);
				auto yjiIdx = indexPairToVarIndex(j, i);
				constraintRow.first[yijIdx] = 1;
				constraintRow.first[yjiIdx] = -1;
			}
		}
	}
}

void
TradeMaximizingSolver::doSolve() {
	while(doPivot()) {}
}

void 
TradeMaximizingSolver::setUpperBound(AssetPair assetPair, int128_t upperBound) {
	auto yIdx = assetPairToVarIndex(assetPair);
	auto eIdx = yIdx + (mNumAssets -1) * (mNumAssets - 1);

	if (mAssetPairToRowMap.find(assetPair) != mAssetPairToRowMap.end()) {
		throw std::runtime_error("can't double-set upper bound");
	}

	if (upperBound <= 0) {
		throw std::runtime_error("can't have nonpositive upper bound");
	}

	auto rowIdx = mCoefficients.size();
	mCoefficients.emplace_back();

	mAssetPairToRowMap[assetPair] = rowIdx;

	auto& row = mCoefficients.back();
	row.first.resize(numVars());
	row.first[yIdx] = 1;
	row.first[eIdx] = 1;
	row.second = upperBound;

	auto& objectiveRow = mCoefficients.front();
	objectiveRow.first[yIdx] = 1;
}

std::optional<size_t>
TradeMaximizingSolver::getNextPivotIndex() const {
	auto& objRow = mCoefficients.front();
	for (size_t i = 0; i < objRow.first.size(); i++) {
		if (objRow.first[i] > 0) {
			return i;
		}
	}
	return std::nullopt;
}

size_t
TradeMaximizingSolver::getNextPivotConstraint(size_t pivotColumn) const {
	size_t minIdx = 0;
	bool foundValue = false;
	int128_t minValue = 0;
	for (size_t i = 1; i < mCoefficients.size(); i++) {
		if (mCoefficients[i].first[pivotColumn] > 0) {
			if (!foundValue || minValue < mCoefficients[i].second) {
				foundValue = true;
				minIdx = i;
				minValue = mCoefficients[i].second;
			}
		}
	}
	if (!foundValue) {
		throw std::runtime_error("degenerate system");
	}
	return minIdx;
}

bool
TradeMaximizingSolver::doPivot() {
	auto nextPivotIdx = getNextPivotIndex();
	if (!nextPivotIdx) {
		return false;
	}
	auto nextPivotConstraint = getNextPivotConstraint(*nextPivotIdx);

	auto coeff = mCoefficients[nextPivotConstraint].first[*nextPivotIdx];
	if (!(coeff == 1 || coeff == -1)) {
		throw std::runtime_error("matrix should be totally unimodular!");
	}
	multiplyRow(nextPivotConstraint, coeff);
	for (size_t i = 0; i < mCoefficients.size(); i++) {
		if (i != nextPivotConstraint) {
			addRowToRow(nextPivotConstraint, i, -coeff);
		}
	}
	return true;
}

void
TradeMaximizingSolver::addRowToRow(size_t rowToAddIdx, size_t rowToBeAddedToIdx, int8_t coefficient) {
	auto& rowToAdd = mCoefficients[rowToAddIdx];
	auto& rowToBeAddedTo = mCoefficients[rowToBeAddedToIdx];

	for (size_t i = 0; i < rowToAdd.first.size(); i++) {
		rowToBeAddedTo.first[i] += rowToAdd.first[i] * coefficient;
	}
	rowToBeAddedTo.second += rowToAdd.second * coefficient;
}

void
TradeMaximizingSolver::multiplyRow(size_t rowIdx, int8_t coefficient) {
	if (coefficient == 1) return;
	auto& row = mCoefficients[rowIdx];

	for (size_t i = 0; i < row.first.size(); i++) {
		row.first[i] *= coefficient;
	}
	row.second *= coefficient;
}

} /* stellar */