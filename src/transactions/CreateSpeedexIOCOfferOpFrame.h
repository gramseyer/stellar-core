#include "transactions/OperationFrame.h"

namespace stellar
{
class AbstractLedgerTxn;
class TransactionCommutativityRequirements;

class CreateSpeedexIOCOfferOpFrame : public OperationFrame
{
    CreateSpeedexIOCOfferResult&
    innerResult()
    {
        return mResult.tr().createSpeedexIOCOfferResult();
    }
    
    CreateSpeedexIOCOfferOp const& mCreateSpeedexIOCOffer;

    uint32_t mOperationIndex;

    bool checkMalformed();
    bool checkValidAssetPair(AbstractLedgerTxn& ltx);

  public:
    CreateSpeedexIOCOfferOpFrame(Operation const& op, OperationResult& res,
                   TransactionFrame& parentTx, uint32_t index);

    bool doApply(AbstractLedgerTxn& ltx) override;
    bool doCheckValid(uint32_t ledgerVersion) override;

    bool doAddCommutativityRequirements(AbstractLedgerTxn& ltx, TransactionCommutativityRequirements& reqs) override;
    void doAddCommutativityRequirementsUnconditional(TransactionCommutativityRequirements& reqs) const override;

    void
    insertLedgerKeysToPrefetch(UnorderedSet<LedgerKey>& keys) const override;

    static CreateSpeedexIOCOfferResultCode
    getInnerCode(OperationResult const& res)
    {
        return res.tr().createSpeedexIOCOfferResult().code();
    }
};

} /* stellar */
