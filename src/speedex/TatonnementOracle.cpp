#include "speedex/TatonnementOracle.h"

#include "speedex/IOCOrderbookManager.h"
#include "speedex/LiquidityPoolSetFrame.h"

#include "util/types.h"

#include "speedex/DemandOracle.h"
#include "speedex/DemandUtils.h"


namespace stellar
{

TatonnementOracle::TatonnementOracle(DemandOracle const& demandOracle)
	: mDemandOracle(demandOracle)
{}

void 
TatonnementOracle::computePrices(TatonnementControlParams const& params, std::map<Asset, uint64_t>& prices, const uint32_t printFrequency)
{
	TatonnementControlParamsWrapper controlParams(params);

	SupplyDemand baselineDemand = mDemandOracle.demandQuery(prices, controlParams.smoothMult());

	TatonnementObjectiveFn baselineObjective = baselineDemand.getObjective();

	uint64_t stepSize = controlParams.kStartingStepSize;

	while (!controlParams.done()) {

		controlParams.incrementRound();

		std::map<Asset, uint64_t> trialPrices = controlParams.setTrialPrices(prices, baselineDemand, stepSize);

		auto trialDemand = mDemandOracle.demandQuery(trialPrices, controlParams.smoothMult());

		TatonnementObjectiveFn trialObjective = trialDemand.getObjective();

		if (trialObjective.isBetterThan(baselineObjective, 1, 100) || stepSize < controlParams.kMinStepSize)
		{
			prices = trialPrices;

			baselineDemand = trialDemand;
			baselineObjective = trialObjective;

			stepSize = controlParams.stepUp(std::max(stepSize, controlParams.kMinStepSize));
		} else {
			stepSize = controlParams.stepDown(stepSize);
		}

		if (printFrequency > 0 && controlParams.getRoundNumber() % printFrequency == 0)
		{
			std::printf("TATONNEMENT STEP: step size: %llu round number: %lu\n", stepSize, controlParams.getRoundNumber());
			for (auto const& [asset, price] : prices)
			{
				int128_t demand = baselineDemand.getDelta(asset);
				auto str = assetToString(asset);
				std::printf("TATONNEMENT: %s\t%15llu\t%lf\n", str.c_str(), price, (double)demand);
			}
		}
	}
}


} /* stellar */
