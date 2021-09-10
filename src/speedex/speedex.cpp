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

SpeedexResults
runSpeedex(AbstractLedgerTxn& ltx)
{

    bool printDiagnostics = false;
    auto& speedexOrderbooks = ltx.getSpeedexIOCOffers();
    speedexOrderbooks.sealBatch();

    auto speedexConfig = loadSpeedexConfigSnapshot(ltx);

    LiquidityPoolSetFrame liquidityPools(speedexConfig.getAssets(), ltx);

    DemandOracle demandOracle(speedexOrderbooks, liquidityPools);

    TatonnementOracle oracle(demandOracle);

    TatonnementControlParams controls = speedexConfig.getControls();
    auto prices = speedexConfig.getStartingPrices();

    oracle.computePrices(controls, prices, printDiagnostics ? 1 : 0);

    if (printDiagnostics)
    {
        std::printf("PRICES\n");
        for (auto const& [asset, price] : prices)
        {
            std::printf("%llu\n", price);
        }
    }

    TradeMaximizingSolver solver(speedexConfig.getAssets());

    demandOracle.setSolverUpperBounds(solver, prices);

    solver.doSolve();

    BatchSolution solution(solver.getSolution(), prices);

    return speedexOrderbooks.clearBatch(ltx, solution, liquidityPools);
}

} /* stellar */