#ifndef KSP_PORKCHOP_TOOLS_H
#define KSP_PORKCHOP_TOOLS_H

#include "transfer_tools.h"

struct Porkchop_Properties {
    double jd_min_dep, jd_max_dep, dep_time_steps, arr_time_steps;
    int min_duration, max_duration;
    struct Ephem **ephems;
    struct Body *dep_body, *arr_body;
};

void create_porkchop(struct Porkchop_Properties pochopro, enum Transfer_Type tt, double *all_data);
double get_min_arr_from_porkchop(double *pc);
double get_max_arr_from_porkchop(double *pc);
double get_min_from_porkchop(double *pc, int index);


#endif //KSP_PORKCHOP_TOOLS_H
