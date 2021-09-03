#include "speedex/TatonnementOracle.h"

#include "speedex/IOCOrderbookManager.h"

#include "util/types.h"


namespace stellar
{

TatonnementOracle::TatonnementOracle(const IOCOrderbookManager& orderbookManager)
	: mOrderbookManager(orderbookManager) 
{}


void 
TatonnementOracle::computePrices(TatonnementControlParams const& params, std::map<Asset, uint64_t>& prices, uint32_t printFrequency)
{
	TatonnementControlParamsWrapper controlParams(params);

	std::map<Asset, int128_t> baselineDemand;

	mOrderbookManager.demandQuery(prices, baselineDemand, controlParams.taxRate(), controlParams.smoothMult());

	TatonnementObjectiveFn baselineObjective(baselineDemand);

	uint64_t stepSize = controlParams.kStartingStepSize;

	while (!controlParams.done()) {

		controlParams.incrementRound();

		std::map<Asset, uint64_t> trialPrices = controlParams.setTrialPrices(prices, baselineDemand, stepSize);
		std::map<Asset, int128_t> trialDemand;

		mOrderbookManager.demandQuery(trialPrices, trialDemand, controlParams.taxRate(), controlParams.smoothMult());

		TatonnementObjectiveFn trialObjective(trialDemand);

		if (trialObjective.isBetterThan(baselineObjective, 1, 100) || stepSize < controlParams.kMinStepSize)
		{
			prices = trialPrices;
			baselineDemand.clear();
			mOrderbookManager.demandQuery(prices, baselineDemand, controlParams.taxRate(), controlParams.smoothMult());
			baselineObjective = TatonnementObjectiveFn(baselineDemand);

			stepSize = controlParams.stepUp(std::max(stepSize, controlParams.kMinStepSize));
		} else {
			stepSize = controlParams.stepDown(stepSize);
		}

		if (printFrequency > 0 && controlParams.getRoundNumber() % printFrequency == 0)
		{
			std::printf("step size: %llu round number: %lu\n", stepSize, controlParams.getRoundNumber());
			for (auto const& [asset, price] : prices)
			{
				int128_t demand = baselineDemand[asset];
				auto str = assetToString(asset);
				std::printf("%s\t%15llu\t%lf\n", str.c_str(), price, (double)demand);
			}
		}
	}
}


} /* stellar */
