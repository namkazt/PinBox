#ifndef _PP_ANIM_H_
#define _PP_ANIM_H_

#include <easing.h>
#include <3ds/services/am.h>
#include <3ds/ndsp/ndsp.h>
#include <cstdlib>

#define TIME_MILISECOND 1Ull
#define TIME_SECOND 1000Ull
#define TIME_MINUTE 3600000Ull
#define TIME_HOUR 216000000Ull

class Anim {
	struct AnimInfo
	{
		u32 idx;
		u64 startTime;
		u64 duration;
		float from;
		float to;
		easing_functions func;
	};
};


#endif