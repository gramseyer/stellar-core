// Copyright 2020 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "transactions/TransactionBridge.h"
#include "transactions/TransactionFrame.h"
#include "util/GlobalChecks.h"
#include "xdr/Stellar-transaction.h"

namespace stellar
{
namespace txbridge
{

TransactionEnvelope
convertForV13(TransactionEnvelope const& input)
{
    if (input.type() != ENVELOPE_TYPE_TX_V0)
    {
        return input;
    }

    auto const& envV0 = input.v0();
    auto const& txV0 = envV0.tx;

    TransactionEnvelope res(ENVELOPE_TYPE_TX);
    auto& envV1 = res.v1();
    auto& txV1 = envV1.tx;

    envV1.signatures = envV0.signatures;
    txV1.sourceAccount.ed25519() = txV0.sourceAccountEd25519;
    txV1.fee = txV0.fee;
    txV1.seqNum = txV0.seqNum;
    txV1.timeBounds = txV0.timeBounds;
    txV1.memo = txV0.memo;
    txV1.operations = txV0.operations;

    return res;
}

xdr::xvector<DecoratedSignature, 20>&
getSignatures(TransactionEnvelope& env)
{
    switch (env.type())
    {
    case ENVELOPE_TYPE_TX_V0:
        return env.v0().signatures;
    case ENVELOPE_TYPE_TX:
        return env.v1().signatures;
    case ENVELOPE_TYPE_TX_FEE_BUMP:
        return env.feeBump().signatures;
    case ENVELOPE_TYPE_TX_COMMUTATIVE:
        return env.commutativeTx().signatures;
    default:
        abort();
    }
}

xdr::xvector<DecoratedSignature, 20>&
getSignaturesInner(TransactionEnvelope& env)
{
    switch (env.type())
    {
    case ENVELOPE_TYPE_TX_V0:
        return env.v0().signatures;
    case ENVELOPE_TYPE_TX:
        return env.v1().signatures;
    case ENVELOPE_TYPE_TX_FEE_BUMP:
        switch(env.feeBump().tx.innerTx.type())
        {
        case ENVELOPE_TYPE_TX:
            return env.feeBump().tx.innerTx.v1().signatures;
        default:
            abort();
        }
    case ENVELOPE_TYPE_TX_COMMUTATIVE:
        return env.commutativeTx().signatures;
    default:
        abort();
    }
}

xdr::xvector<Operation, MAX_OPS_PER_TX>&
getOperations(TransactionEnvelope& env)
{
    switch (env.type())
    {
    case ENVELOPE_TYPE_TX_V0:
        return env.v0().tx.operations;
    case ENVELOPE_TYPE_TX:
        return env.v1().tx.operations;
    case ENVELOPE_TYPE_TX_FEE_BUMP:
        switch(env.feeBump().tx.innerTx.type())
        {
        case ENVELOPE_TYPE_TX:
            return env.feeBump().tx.innerTx.v1().tx.operations;
        default:
            abort();
        }
    case ENVELOPE_TYPE_TX_COMMUTATIVE:
        return env.commutativeTx().tx.operations;
    default:
        abort();
    }
}

#ifdef BUILD_TESTS
xdr::xvector<DecoratedSignature, 20>&
getSignatures(TransactionFramePtr tx)
{
    return getSignatures(tx->getEnvelope());
}

struct SeqNumAccessor {
    using ReturnT = int64_t;

    template<typename TxType>
    static ReturnT& get(TxType& tx) {
        return tx.seqNum;
    }
};

struct MemoAccessor {
    using ReturnT = Memo;

    template<typename TxType>
    static ReturnT& get(TxType& tx) {
        return tx.memo;
    }
};

struct FeeAccessor {
    using ReturnT = uint32_t;

    template<typename TxType>
    static ReturnT& get(TxType& tx) {
        return tx.fee;
    }
};

struct TimeBoundsAccessor {
    using ReturnT = xdr::pointer<TimeBounds>;

    template<typename TxType>
    static ReturnT& get(TxType& tx) {
        return tx.timeBounds;
    }
};

template<typename AccessorT>
typename AccessorT::ReturnT& genericEnvelopeAccess(TransactionEnvelope& env)
{
    switch(env.type())
    {
    case ENVELOPE_TYPE_TX_V0:
        return AccessorT::template get<TransactionV0>(env.v0().tx);
    case ENVELOPE_TYPE_TX:
        return AccessorT::template get<Transaction>(env.v1().tx);
    case ENVELOPE_TYPE_TX_COMMUTATIVE:
        return AccessorT::template get<CommutativeTransaction>(env.commutativeTx().tx);
    default:
        abort();
    }
}

void
setSeqNum(TransactionFramePtr tx, int64_t seq)
{
    auto& env = tx->getEnvelope();
    genericEnvelopeAccess<SeqNumAccessor>(env) = seq;
}

void
setFee(TransactionFramePtr tx, uint32_t fee)
{
    auto& env = tx->getEnvelope();
    genericEnvelopeAccess<FeeAccessor>(env) = fee;
}

void
setMemo(TransactionFramePtr tx, Memo memo)
{
    auto& env = tx->getEnvelope();
    genericEnvelopeAccess<MemoAccessor>(env) = memo;
}

void
setMinTime(TransactionFramePtr tx, TimePoint minTime)
{
    auto& env = tx->getEnvelope();
    auto& tb = genericEnvelopeAccess<TimeBoundsAccessor>(env);
    tb.activate().minTime = minTime;
}

void
setMaxTime(TransactionFramePtr tx, TimePoint maxTime)
{
    auto& env = tx->getEnvelope();
    auto& tb = genericEnvelopeAccess<TimeBoundsAccessor>(env);
    tb.activate().maxTime = maxTime;
}
#endif
}
}
