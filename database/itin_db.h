//
// Created by niklas on 17.03.24.
//

#ifndef KSP_ITIN_DB_H
#define KSP_ITIN_DB_H

#include "orbit_calculator/transfer_tools.h"

void init_itin_db();
void close_itin_db();
int store_itineraries_in_db(struct ItinStep **departures, int num_deps, const char *itin_name);

#endif //KSP_ITIN_DB_H
