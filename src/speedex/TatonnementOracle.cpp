#include "speedex/TatonnementOracle.h"

#include "speedex/IOCOrderbookManager.h"


namespace stellar
{

void 
TatonnementOracle::computePrices(TatonnementControlParams const& params, std::map<Asset, uint64_t>& prices)
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
			mOrderbookManager.demandQuery(prices, baselineDemand, controlParams.taxRate(), controlParams.smoothMult());
			baselineObjective = TatonnementObjectiveFn(baselineDemand);

			stepSize = controlParams.stepUp(stepSize);
		} else {
			stepSize = controlParams.stepDown(stepSize);
		}
	}
}


} /* stellar */
