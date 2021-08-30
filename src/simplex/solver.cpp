#include "simplex/solver.h"


namespace std {

template <> class hash<std::pair<size_t, size_t>>
{
	// Used in this class, pairs do not have pair.first == pair.second,
	// so this hash is ok
  public:
    size_t
    operator()(std::pair<size_t, size_t> const& pair) const
    {
    	std::hash<size_t> hasher;
        return hasher(pair.first) ^ (hasher(pair.second));
    }
};

}

namespace stellar {


size_t
TradeMaximizingSolver::numVars() const {
	return numYijEijVars() + mNumAssets;
}

size_t
TradeMaximizingSolver::numYijEijVars() const {
	return numYijVars() * 2;
}

size_t
TradeMaximizingSolver::numYijVars() const {
	return (mNumAssets - 1) * (mNumAssets);
}

size_t
TradeMaximizingSolver::indexPairToVarIndex(size_t sell, size_t buy) const {
	return sell * (mNumAssets - 1) + (buy > sell ? buy - 1 : buy);
}

std::optional<std::pair<size_t, size_t>> 
TradeMaximizingSolver::varIndexToIndexPair(size_t yijIdx) const {
	if (yijIdx > numYijVars()) {
		return std::nullopt;
	}
	auto sellIdx = yijIdx / (mNumAssets - 1);
	auto buyIdx = yijIdx % (mNumAssets - 1);
	if (buyIdx >= sellIdx) {
		buyIdx ++;
	}
	return {{sellIdx, buyIdx}};
}

void
TradeMaximizingSolver::printRow(size_t rowIdx) const {
	for (size_t i = 0; i < numVars(); i++) {
		auto var = mCoefficients[rowIdx].first[i];
		if (var < 0) {
			std::printf("-%d ", -var);
		} else {
			std::printf(" %d ", var);
		}
	}
	double coeff = (double) mCoefficients[rowIdx].second;
	std::printf("%lf\n", coeff);
}

void
TradeMaximizingSolver::printTableau() const
{
	for (size_t i = 0; i < mCoefficients.size(); i++) {
		printRow(i);
	}
}


size_t 
TradeMaximizingSolver::assetPairToVarIndex(AssetPair assetPair) const {
	auto sellIdx = mIndexMap.at(assetPair.selling);
	auto buyIdx = mIndexMap.at(assetPair.buying);
	return indexPairToVarIndex(sellIdx, buyIdx);
}

void 
TradeMaximizingSolver::throwIfSolved() const
{
	if (mSolved) {
		throw std::logic_error("already solved");
	}
}

void 
TradeMaximizingSolver::throwIfUnsolved() const
{
	if (!mSolved)
	{
		throw std::logic_error("not yet solved");
	}
}

TradeMaximizingSolver::TradeMaximizingSolver(std::vector<Asset> assets) 
	: mSolved(false) 
{
	size_t idx = 0;
	for (auto asset : assets) {
		mIndexMap[asset] = idx;
	//	mReverseIndexMap[idx] = asset;
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
		row_idx_t row_idx = i + 1;
		auto& constraintRow = mCoefficients[row_idx];
		for (size_t j = 0; j < mNumAssets; j++) {
			if (i != j) {
				auto yijIdx = indexPairToVarIndex(i, j);
				auto yjiIdx = indexPairToVarIndex(j, i);
				constraintRow.first[yijIdx] = 1;
				constraintRow.first[yjiIdx] = -1;
			}
		}
		// Sum_{j} y_ij - Sum_{j} y_ji >= 0
		// Sum_{j} y_ij - Sum_{j} y_ji - slack == 0
		// amount of i sold to market >= amount of i bought from market.
		// amount of i sold to market  - slack var = amount of i bought from market
		size_t slackVarIdx = numYijEijVars() + i;
		constraintRow.first[slackVarIdx] = 1;
		mActiveBasis[row_idx] = slackVarIdx;
	}

	mActiveYijs.resize(numYijVars(), false);

	std::printf("start tableau:\n");
	printTableau();
}

void
TradeMaximizingSolver::doSolve() {
	throwIfSolved();
	while(doPivot()) {}
	constructSolution();
	mSolved = true;
}

void 
TradeMaximizingSolver::setUpperBound(AssetPair assetPair, int128_t upperBound)
{
	throwIfSolved();
	auto yIdx = assetPairToVarIndex(assetPair);
	auto eIdx = yIdx + numYijVars();

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

	//std::printf("%lf\n", (double) upperBound);

	mActiveBasis[rowIdx] = eIdx;
	mActiveYijs[yIdx] = true;

	auto& objectiveRow = mCoefficients.front();
	objectiveRow.first[yIdx] = 1;

	//printRow(mCoefficients.size() -1 );

	std::printf("post add tableau:\n");
	printTableau();
}

std::optional<size_t>
TradeMaximizingSolver::getNextPivotIndex() const {
	auto& objRow = mCoefficients.front();
	for (size_t i = 0; i < objRow.first.size(); i++) {
		if (objRow.first[i] > 0 && mActiveYijs[i]) {
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
		std::printf("next pivot check %lu\n", i);
		if (mCoefficients[i].first[pivotColumn] > 0) {
			std::printf("coeff is positive foundValue = %lu minValue = %lf\n", foundValue, (double) minValue);
			if ((!foundValue) || minValue > mCoefficients[i].second) {
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
	std::printf("doing pivot\n");
	printTableau();
	auto nextPivotIdx = getNextPivotIndex();
	if (!nextPivotIdx) {
		return false;
	}
	auto nextPivotConstraint = getNextPivotConstraint(*nextPivotIdx);

	auto coeff = mCoefficients[nextPivotConstraint].first[*nextPivotIdx];
	if (!(coeff == 1 || coeff == -1)) {
		throw std::runtime_error("matrix should be totally unimodular!");
	}

	std::printf("pivot col %lu pivot row %lu coeff %d\n", *nextPivotIdx, nextPivotConstraint, coeff);
	multiplyRow(nextPivotConstraint, coeff);
	std::printf("post mult\n");
	printTableau();
	for (size_t i = 0; i < mCoefficients.size(); i++) {
		if (i != nextPivotConstraint) {
			auto rowCoeff = -coeff * mCoefficients[i].first[*nextPivotIdx];
			addRowToRow(nextPivotConstraint, i, rowCoeff);
		}
	}

	mActiveBasis[nextPivotConstraint] = *nextPivotIdx;


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

TradeMaximizingSolver::int128_t 
TradeMaximizingSolver::getRowResult(AssetPair assetPair) const {
	throwIfUnsolved();
	auto pair = std::make_pair(mIndexMap.at(assetPair.selling), mIndexMap.at(assetPair.buying));
	std::printf("query for pair %lu %lu\n", pair.first, pair.second);
	return mSolutionMap.at(pair);
}

void
TradeMaximizingSolver::constructSolution()
{
	UnorderedMap<size_t, UnorderedSet<std::pair<size_t, size_t>>> unsolvedVariables;

	std::vector<bool> solvedYijs;
	solvedYijs.resize(numYijVars(), false);

	for (auto const& [row_idx, col_idx] : mActiveBasis)
	{
		auto assetIdxs = varIndexToIndexPair(col_idx);
		if (assetIdxs) {
			solvedYijs[col_idx] = true;
			mSolutionMap[*assetIdxs] = mCoefficients[row_idx].second;
		}
	}

	for (auto yij = 0u; yij < solvedYijs.size(); yij++) {
		if (!mActiveYijs[yij]) {
			solvedYijs[yij] = true;
			auto indexPair = *varIndexToIndexPair(yij);
			mSolutionMap[indexPair] = 0;
			std::printf("adding yij=0 to solution bc no upper bd set idx %lu (%lu %lu) \n", yij, indexPair.first, indexPair.second);
		}
		else if (!solvedYijs[yij])
		{
			auto [sell, buy] = *varIndexToIndexPair(yij);
			unsolvedVariables[sell].insert({sell, buy});
			unsolvedVariables[buy].insert({sell, buy});
		}
	}

	//TODO sketchy
	while (!unsolvedVariables.empty())
	{
		bool madeProgress = false;
		for (auto iter = unsolvedVariables.begin(); iter != unsolvedVariables.end(); iter++)
		{
			auto& [asset, unsolvedVarSet] = *iter;
			if (unsolvedVarSet.size() == 1)
			{
				madeProgress = true;
				auto [sell, buy] = *unsolvedVarSet.begin();
				if (sell == asset) {
					int128_t totalOtherSold = 0;
					for (size_t otherBuy = 0; otherBuy < mNumAssets; otherBuy ++) {
						if (otherBuy != buy)
						{
							totalOtherSold += mSolutionMap[{asset, otherBuy}];
						}
					}
					int128_t totalBought = 0;
					for (size_t otherSell = 0; otherSell < mNumAssets; otherSell ++) {
						totalBought += mSolutionMap[{otherSell, asset}];
					}
					mSolutionMap[{sell, buy}] = totalBought - totalOtherSold;
					if (mSolutionMap[{sell, buy}] < 0) {
						throw std::logic_error("can't get a negative yij");
					}
					
				} else {
					int128_t totalOtherBought = 0;
					for (size_t otherSell = 0; otherSell < mNumAssets; otherSell ++) {
						if (otherSell != sell)
						{
							totalOtherBought += mSolutionMap[{otherSell, asset}];
						}
					}
					int128_t totalSold = 0;
					for (size_t otherBuy = 0; otherBuy < mNumAssets; otherBuy ++) {
						totalSold += mSolutionMap[{asset, otherBuy}];
					}
					mSolutionMap[{sell, buy}] = totalSold - totalOtherBought;
					if (mSolutionMap[{sell, buy}] < 0) {
						throw std::logic_error("can't get a negative yij");
					}
				}
				unsolvedVariables.erase(iter);
				auto sellIter = unsolvedVariables.find(sell);
				if (sellIter != unsolvedVariables.end()) {
					auto& [_, oppositeUnsolvedSet] = *sellIter;
					oppositeUnsolvedSet.erase({sell, buy});
					if (oppositeUnsolvedSet.size() == 0) {
						throw std::logic_error("unsolvedSet empty?");
					}
				}
				auto buyIter = unsolvedVariables.find(buy);
				if (buyIter != unsolvedVariables.end()) {
					auto& [_, oppositeUnsolvedSet] = *buyIter;
					oppositeUnsolvedSet.erase({sell, buy});
					if (oppositeUnsolvedSet.size() == 0) {
						throw std::logic_error("unsolvedSet empty?");
					}
				}

				break;
			}
		}
		if (!madeProgress) {
			throw std::runtime_error("stuck!");
		}
	}
}

} /* stellar */