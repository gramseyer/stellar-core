#pragma once

#include "xdr/Stellar-ledger.h"

namespace stellar
{

class AbstractLedgerTxn;

SpeedexResults
runSpeedex(AbstractLedgerTxn& ltx);

} /* stellar */