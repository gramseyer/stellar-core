#include "speedex/IOCOffer.h"

#include "util/XDROperators.h"

#include <xdrpp/marshal.h>

#include "xdr/Stellar-transaction.h"
#include "crypto/SHA.h"

#include "ledger/LedgerTxn.h"

#include "ledger/TrustLineWrapper.h"

#include "transactions/TransactionUtils.h"

namespace stellar {

IOCOffer::IOCOffer(int64_t sellAmount, Price minPrice, Hash totalOrderingHash, AccountID sourceAccount)
	: mSellAmount(sellAmount), mMinPrice(minPrice), mTotalOrderingHash(totalOrderingHash), mSourceAccount(sourceAccount) {}

std::strong_ordering 
IOCOffer::operator<=>(const IOCOffer& other) const {
	int64_t lhs = ((int64_t)mMinPrice.n) * ((int64_t) other.mMinPrice.d);
	int64_t rhs = ((int64_t)mMinPrice.d) * ((int64_t) other.mMinPrice.n);

	if (lhs < rhs) {
		return std::strong_ordering::less;
	}
	if (lhs > rhs) {
		return std::strong_ordering::greater;
	}

	if (mTotalOrderingHash < other.mTotalOrderingHash) {
		return std::strong_ordering::less;
	}
	if (mTotalOrderingHash > other.mTotalOrderingHash) {
		return std::strong_ordering::greater;
	}
	return std::strong_ordering::equal;
}

//should not be changed by fee bump tx
Hash 
IOCOffer::offerHash(Price price, AccountID sourceAccount, uint64_t sourceSeqNum, uint32_t opIdNum)
{
	SHA256 hasher;

	SpeedexIOCOfferHashContents hashContents(
		sourceAccount,
		price,
		sourceSeqNum,
		opIdNum);

	hasher.add(xdr::xdr_to_opaque(hashContents));
	return hasher.finish();
}

void IOCOffer::unwindOffer(AbstractLedgerTxn& ltx, const Asset& sellAsset) const {
	

	/*auto header = ltx.loadHeader();
	auto account = loadAccount(ltx, mSourceAccount);


	if (sellAsset.type() == ASSET_TYPE_NATIVE) {

		if (!addBalance(header, account, mSellAmount)) {
			throw std::runtime_error("couldn't refund ioc trade");
		}
	} else {
		auto sourceLine = loadTrustLine(ltx, mSourceAccount, sellAsset);

    	if (!sourceLine)
        {
            throw std::runtime_error("couldn't refund ioc bc no trustline");
        }

        if (!sourceLine.addBalance(header, mSellAmount))
        {
            throw std::runtime_error("couldn't refund ioc trustline balance");
        }
    }*/
}




} /* stellar */