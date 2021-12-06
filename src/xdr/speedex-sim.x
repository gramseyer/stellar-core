%#include "Stellar-types.h"
%#include "Stellar-ledger-entries.h"

namespace stellar {

struct AMMConfig {
	AssetCode12 assetA;
	AssetCode12 assetB;

	int64 amountA;
	int64 amountB;
};

struct SpeedexSimConfig {
	AssetCode12 assets<>;

	AMMConfig ammConfigs<>;
};

struct SpeedexOffer {

	uint64 offerID;
	Price minPrice;
	AssetCode12 selling;
	AssetCode12 buying;	
	int64_t amount;
};

struct SpeedexSimulation {
	SpeedexSimConfig config;

	SpeedexOffer offers<>;
};



	
} /* stellar */
