// Copyright 2021 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

%#include "xdr/Stellar-types.h"
%#include "xdr/Stellar-ledger-entries.h"

namespace stellar {

struct SpeedexOfferClearingStatus
{
    Asset sellAsset;
    Asset buyAsset;

    AccountID sourceAccount;
    uint64 seqNum;
    uint32 offerIndex;

    int64 soldAmount;
    int64 boughtAmount;
};

struct SpeedexLiquidityPoolClearingStatus
{
    PoolID pool;
    Asset soldAsset;
    Asset boughtAsset;
    int64 soldAmount;
    int64 boughtAmount;
};

struct SpeedexClearingValuation
{
    Asset asset;
    uint64 price;
};

struct SpeedexResults {
    SpeedexClearingValuation valuations<>;
    SpeedexOfferClearingStatus offerStatuses<>;
    SpeedexLiquidityPoolClearingStatus lpStatuses<>;
};

	
} /* stellar */
