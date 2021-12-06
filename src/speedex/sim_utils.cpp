#include "speedex/sim_utils.h"

namespace stellar {

Asset makeSimAsset(AssetCode12 const& code)
{
    Asset out;
    out.type(ASSET_TYPE_CREDIT_ALPHANUM12);
    out.alphaNum12().assetCode = code;
    //issuer unset, we don't care about that here
    return out;
}

} /* stellar */
