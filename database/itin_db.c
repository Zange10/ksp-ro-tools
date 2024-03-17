//
// Created by niklas on 17.03.24.
//

#include "itin_db.h"
#include "database.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include "tools/tool_funcs.h"


sqlite3 *itin_db;

int store_step_in_db(struct ItinStep *step, int prev_step_id) {
	char query[500];
	if(prev_step_id >= 0) {
		sprintf(query, "INSERT INTO Step (prevStepID, Date, BodyID, RX, RY, RZ, "
					   "VdepX, VdepY, VdepZ, VarrX, VarrY, VarrZ, VbodX, VbodY, VbodZ) "
					   "VALUES (%d, %f, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f);",
				prev_step_id,
				step->date,
				step->body->id,
				step->r.x, step->r.y, step->r.z,
				step->v_dep.x, step->v_dep.y, step->v_dep.z,
				step->v_arr.x, step->v_arr.y, step->v_arr.z,
				step->v_body.x, step->v_body.y, step->v_body.z);
	} else {
		sprintf(query, "INSERT INTO Step (prevStepID, Date, BodyID, RX, RY, RZ, "
					   "VdepX, VdepY, VdepZ, VarrX, VarrY, VarrZ, VbodX, VbodY, VbodZ) "
					   "VALUES (NULL, %f, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f);",
				step->date,
				step->body->id,
				step->r.x, step->r.y, step->r.z,
				step->v_dep.x, step->v_dep.y, step->v_dep.z,
				step->v_arr.x, step->v_arr.y, step->v_arr.z,
				step->v_body.x, step->v_body.y, step->v_body.z);
	}
	if(execute_query(itin_db, query) != SQLITE_OK) return -1;
	else return 0;
}

int store_itinsteps_in_db(struct ItinStep *step, int prev_step_id, int first_node) {
	if(!first_node) {
		if(store_step_in_db(step, prev_step_id) == -1) return -1;
		prev_step_id = (int) sqlite3_last_insert_rowid(itin_db);
	}
	for(int i = 0; i < step->num_next_nodes; i++) {
		if(store_itinsteps_in_db(step->next[i], prev_step_id, 0) == -1) return -1;
	}
	return 0;
}

int store_itineraries_in_db(struct ItinStep **departures, int num_deps, const char *itin_name) {
	char query[500];
	sprintf(query, "INSERT INTO Itineraries (Name) "
				   "VALUES ('%s');", itin_name);
	if(execute_query(itin_db, query) != SQLITE_OK) return -1;
	int itin_id = (int) sqlite3_last_insert_rowid(itin_db);

	for(int i = 0; i < num_deps; i++) {
		struct ItinStep *first_node = departures[i];
		store_step_in_db(first_node, -1);
		int first_node_id = (int) sqlite3_last_insert_rowid(itin_db);

		sprintf(query, "INSERT INTO 'Itin_Step' (ItinID, StepID) "
					   "VALUES (%d, %d);", itin_id, first_node_id);
		if(execute_query(itin_db, query) != SQLITE_OK) return -1;

		if(store_itinsteps_in_db(first_node, first_node_id, 1) == -1) return -1;
		show_progress("Store Itins in database: ", i, num_deps);
	}

	show_progress("Store Itins in database: ", 1, 1);
	printf("\n");
	return 0;
}

void init_itin_db() {
	int rc = sqlite3_open("itin_test.db", &itin_db);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(itin_db));
		return;
	}

	const char *opened_path = sqlite3_db_filename(itin_db, "main");
	if (opened_path != NULL) {
		printf("Opened database: %s\n", opened_path);
	} else {
		printf("Unable to retrieve opened database path.\n");
	}
}

void close_itin_db() {
	sqlite3_close(itin_db);
}