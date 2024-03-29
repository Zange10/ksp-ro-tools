#ifndef KSP_DOUBLE_SWING_BY_H
#define KSP_DOUBLE_SWING_BY_H

#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "transfer_tools.h"


struct DSB {
	struct OSV osv[4];
	double man_time;
	double dv;
};


struct DSB calc_double_swing_by(struct OSV s0, struct OSV p0, struct OSV s1, struct OSV p1, double transfer_duration, struct Body *body);



#endif //KSP_DOUBLE_SWING_BY_H
