#pragma once

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"
#include "xdr/Stellar-ledger.h"

#include <compare>

#include <cstdint>

namespace stellar {

struct AssetPair;

class AbstractLedgerTxn;

struct IOCOffer {
	int64_t mSellAmount;
	Price mMinPrice;
	Hash mTotalOrderingHash;
	AccountID mSourceAccount;
	uint64_t mSourceSeqNum;
	uint32_t mOpIdx;

	IOCOffer(int64_t sellAmount, Price minPrice, AccountID sourceAccount, uint64_t sourceSeqNum, uint32_t opIdNum);

	std::strong_ordering operator<=>(const IOCOffer& other) const;

	// should not be changed by feeBumpTx;
	static Hash offerHash(Price price, AccountID sourceAccount, uint64_t sourceSeqNum, uint32_t opIdNum);

	SpeedexOfferClearingStatus getClearingStatus(int64_t sellAmount, int64_t buyAmount, AssetPair const& tradingPair) const;
};

} /* stellar */