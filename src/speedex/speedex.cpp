#include "speedex/speedex.h"

#include "ledger/LedgerTxn.h"
#include "simplex/solver.h"
#include "speedex/DemandOracle.h"
#include "speedex/LiquidityPoolSetFrame.h"
#include "speedex/TatonnementControls.h"
#include "speedex/TatonnementOracle.h"
#include "speedex/SpeedexConfigEntryFrame.h"
#include "speedex/sim_utils.h"

#include "transactions/TransactionUtils.h"

#include "util/XDROperators.h"

namespace stellar 
{

SpeedexResults
runSpeedex(AbstractLedgerTxn& ltx)
{

    bool printDiagnostics = true;
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

SpeedexConfigEntry
makeConfigSim(SpeedexSimulation const& sim)
{
    SpeedexConfigEntry out;
    for (auto const& assetcode : sim.config.assets) 
    {
        out.speedexAssets.push_back(makeSimAsset(assetcode));
    }
    return out;
}

void checkSimOffer(SpeedexOffer const& offer, SpeedexConfigEntry const& config)
{
    auto validAsset = [&] (AssetCode12 const& code) -> bool {
        for (auto const& configCode : config.speedexAssets) {
            if (configCode.alphaNum12().assetCode == code) {
                return true;
            }
        }
        return false;
    };

    if (!validAsset(offer.selling)) {
        throw std::runtime_error("invalid asset");
    }
    if (!validAsset(offer.buying)) {
        throw std::runtime_error("invalid asset");
    }
    if (offer.amount <= 0) {
        throw std::runtime_error("invalid offer amount");
    }
    if (offer.minPrice.d == 0) {
        throw std::runtime_error("invalid price");
    }
}

void checkSimOffers(SpeedexSimulation const& sim, SpeedexConfigEntry const& config)
{
    for (auto const& offer : sim.offers) {
        checkSimOffer(offer, config);
    }
}

std::pair<AssetPair, IOCOffer> makeOffer(SpeedexOffer const& offer, uint64_t idnum)
{
    AssetPair pair{
        .selling = makeSimAsset(offer.selling),
        .buying = makeSimAsset(offer.buying)
    };

    IOCOffer offer_out(offer.amount, offer.minPrice, AccountID{}, idnum, 0);
    return {pair, offer_out};
}

IOCOrderbookManager
makeOrderbooks(SpeedexSimulation const& sim)
{
    IOCOrderbookManager manager;
    auto const& offers = sim.offers;
    for (auto i = 0u; i < offers.size(); i++) {
        auto [pair, offer] = makeOffer(offers[i], i);
        manager.addOffer(pair, offer);
    }
    return manager;
}

void
runSpeedexSim(SpeedexSimulation const& sim)
{
    SpeedexConfigEntry speedexConfig = makeConfigSim(sim);
    checkSimOffers(sim, speedexConfig);

    auto orderbooks = makeOrderbooks(sim);

    LiquidityPoolSetFrame lps(sim);

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

    return orderbooks.clearSimBatch(solution, liquidityPools);
}

} /* stellar */