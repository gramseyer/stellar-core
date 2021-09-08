#include "speedex/speedex.h"

#include "ledger/LedgerTxn.h"
#include "simplex/solver.h"
#include "speedex/DemandOracle.h"
#include "speedex/LiquidityPoolSetFrame.h"
#include "speedex/TatonnementControls.h"
#include "speedex/TatonnementOracle.h"
#include "speedex/SpeedexConfigEntryFrame.h"

#include "transactions/TransactionUtils.h"

namespace stellar 
{
void
runSpeedex(AbstractLedgerTxn& ltx)
{
    auto& speedexOrderbooks = ltx.getSpeedexIOCOffers();
    speedexOrderbooks.sealBatch();

    auto speedexConfig = loadSpeedexConfigSnapshot(ltx);

    LiquidityPoolSetFrame liquidityPools(speedexConfig.getAssets(), ltx);

    DemandOracle demandOracle(speedexOrderbooks, liquidityPools);

    TatonnementOracle oracle(demandOracle);

    TatonnementControlParams controls = speedexConfig.getControls();
    auto prices = speedexConfig.getStartingPrices();

    oracle.computePrices(controls, prices);

    TradeMaximizingSolver solver(speedexConfig.getAssets());

    demandOracle.setSolverUpperBounds(solver, prices);

    BatchSolution solution(solver.getSolution(), prices);

    speedexOrderbooks.clearBatch(ltx, solution);
}

} /* stellar */