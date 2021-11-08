// Copyright 2021 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

%#include "xdr/Stellar-types.h"

namespace stellar
{
	enum TatonnementParam
	{
		MIN_STEP_SIZE = 0,
		SMOOTHNESS_PARAM = 1,
		HEURISTIC_FN = 2
		// ...
	};

	enum TatonnementHeuristic
	{
		PRICE_WEIGHTED_L2_NORM = 0
		// ...
	};

	union TatonnementConfig switch(TatonnementParam param)
	{
	case MIN_STEP_SIZE:
		uint32 minStepSize;
	case SMOOTHNESS_PARAM:
		uint32 smoothnessParam;
	case HEURISTIC_FN:
		TatonnementHeuristic heuristic;
	default:
		void;
	};

	typedef TatonnementConfig TatonnementConfigs<>;

} /* stellar */
