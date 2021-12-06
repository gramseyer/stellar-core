#pragma once

#include "xdr/Stellar-ledger.h"
#include "xdr/speedex-sim.h"

namespace stellar
{

class AbstractLedgerTxn;

SpeedexResults
runSpeedex(AbstractLedgerTxn& ltx);

} /* stellar */