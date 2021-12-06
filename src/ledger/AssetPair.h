#pragma once

#include "xdr/Stellar-ledger-entries.h"

namespace stellar {

struct AssetPair
{
    Asset buying;
    Asset selling;

    AssetPair reverse() const {
        return AssetPair
        {
            .buying = selling,
            .selling = buying
        };
    }
};
bool operator==(AssetPair const& lhs, AssetPair const& rhs);

struct AssetPairHash
{
    size_t operator()(AssetPair const& key) const;
};

} /* stellar */

