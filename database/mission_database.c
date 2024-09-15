#include "mission_database.h"
#include "database.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



void db_new_program(const char *program_name, const char *vision) {
	char query[500];
	sprintf(query, "INSERT INTO Program (Name, Vision) "
				   "VALUES ('%s', '%s');", program_name, vision);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! Program insert Error !!!!!!!!\n");
}

void db_new_mission(const char *mission_name, int program_id) {
	char query[500];
	sprintf(query, "INSERT INTO Mission_DB (Name, ProgramID) "
				   "VALUES ('%s', %d);", mission_name, program_id);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! Mission insert Error !!!!!!!!\n");
}

int db_get_missions_ordered_by_launch_date(struct Mission_DB **p_missions, struct Mission_Filter filter) {
	char query[550];
	char string_programId[20];
	if(filter.program_id == 0) sprintf(string_programId, "");
	else sprintf(string_programId, "AND ProgramID = %d\n", filter.program_id);

	sprintf(query, "SELECT * FROM\n"
				   "    Mission m\n"
				   "LEFT JOIN (\n"
				   "    SELECT\n"
				   "        m.MissionID,\n"
				   "        MIN(me.Time) AS FirstEventDate\n"
				   "    FROM\n"
				   "        Mission m\n"
				   "    LEFT JOIN\n"
				   "        MissionEvent me\n"
				   "    ON\n"
				   "        m.MissionID = me.MissionID\n"
				   "    GROUP BY\n"
				   "        m.MissionID\n"
				   ") AS fe\n"
				   "ON\n"
				   "    m.MissionID = fe.MissionID\n"
				   "WHERE\n"
				   "    m.Name LIKE '%%%s%'\n"
				   "    AND (m.Status = %d OR m.Status = %d OR m.Status = %d)\n"
				   "	%s"
				   "ORDER BY\n"
				   "    CASE\n"
				   "        WHEN fe.FirstEventDate IS NULL THEN 1\n"
				   "        ELSE 0\n"
				   "    END,\n"
				   "    fe.FirstEventDate;", filter.name, filter.ytf?0:-1, filter.in_prog?1:-1, filter.ended?2:-1, string_programId);


	sqlite3_stmt *stmt = execute_multirow_request(query);
	int rc;

	int index = 0;
	int max_size = 4;
	struct Mission_DB *missions = (struct Mission_DB *) malloc(max_size*sizeof(struct Mission_DB));
	*p_missions = missions;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			const char *columnName = sqlite3_column_name(stmt, i);

			if 			(strcmp(columnName, "MissionID") == 0) {
				missions[index].id = sqlite3_column_int(stmt, i);
			} else if 	(strcmp(columnName, "ProgramID") == 0) {
				missions[index].program_id = sqlite3_column_int(stmt, i);
			} else if 	(strcmp(columnName, "Name") == 0) {
				sprintf(missions[index].name, (char *) sqlite3_column_text(stmt, i));
			} else if 	(strcmp(columnName, "Status") == 0) {
				int status = sqlite3_column_int(stmt, i);
				missions[index].status = status == 0 ? YET_TO_FLY : ((status == 1) ? IN_PROGRESS : ENDED);
			} else if 	(strcmp(columnName, "LauncherID") == 0) {
				missions[index].launcher_id = sqlite3_column_int(stmt, i);	// returns 0 if NULL
			} else if 	(strcmp(columnName, "PlaneID") == 0) {
				missions[index].plane_id = sqlite3_column_int(stmt, i);	// returns 0 if NULL
			} else {
				//fprintf(stderr, "!!!!!! %s not yet implemented for Mission !!!!!\n", columnName);
			}
		}

		index++;
		if(index >= max_size) {
			max_size *= 2;
			struct Mission_DB *temp = (struct Mission_DB *) realloc(missions, max_size*sizeof(struct Mission_DB));
			if(temp == NULL) {
				fprintf(stderr, "!!!!!! Reallocation failed for missions from database !!!!!!!!!!");
				return index;
			}
			missions = temp;
			*p_missions = missions;
		}
	}



	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Error stepping through the result set");
	}

	sqlite3_finalize(stmt);

	return index;
}

int db_get_number_of_programs() {
	char query[255];

	sprintf(query, "SELECT COUNT(*) FROM Program;");
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return 0;

	int num_profiles = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);

	return num_profiles;
}

int db_get_all_programs(struct MissionProgram_DB **p_programs) {
	char query[500];
	sprintf(query, "SELECT * FROM Program");


	sqlite3_stmt *stmt = execute_multirow_request(query);
	int rc;

	int index = 0;
	int max_size = 10;
	struct MissionProgram_DB *programs = (struct MissionProgram_DB *) malloc(max_size*sizeof(struct MissionProgram_DB));
	*p_programs = programs;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			const char *columnName = sqlite3_column_name(stmt, i);

			if 			(strcmp(columnName, "ProgramID") == 0) {
				programs[index].id = sqlite3_column_int(stmt, i);
			} else if 	(strcmp(columnName, "Name") == 0) {
				sprintf(programs[index].name, (char *) sqlite3_column_text(stmt, i));
			} else if 	(strcmp(columnName, "Vision") == 0) {
				sprintf(programs[index].vision, (char *) sqlite3_column_text(stmt, i));
			} else {
				fprintf(stderr, "!!!!!! %s not yet implemented for Mission !!!!!\n", columnName);
			}
		}
		index++;
		if(index >= max_size) {
			max_size *= 2;
			struct MissionProgram_DB *temp = (struct MissionProgram_DB *) realloc(programs, max_size*sizeof(struct MissionProgram_DB));
			if(temp == NULL) {
				fprintf(stderr, "!!!!!! Reallocation failed for missions from database !!!!!!!!!!");
				return index;
			}
			programs = temp;
			*p_programs = programs;
		}
	}

	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Error stepping through the result set");
	}

	sqlite3_finalize(stmt);

	return index;
}

struct MissionProgram_DB db_get_program_from_id(int id) {
	char query[48];
	sprintf(query, "SELECT * FROM Program WHERE ProgramID = %d", id);
	struct MissionProgram_DB program = {-1};

	sqlite3_stmt *stmt = execute_single_row_request(query);
	int rc;

	for (int i = 0; i < sqlite3_column_count(stmt); i++) {
		const char *columnName = sqlite3_column_name(stmt, i);

		if 			(strcmp(columnName, "ProgramID") == 0) {
			program.id = sqlite3_column_int(stmt, i);
		} else if 	(strcmp(columnName, "Name") == 0) {
			sprintf(program.name, (char *) sqlite3_column_text(stmt, i));
		} else if 	(strcmp(columnName, "Vision") == 0) {
			sprintf(program.vision, (char *) sqlite3_column_text(stmt, i));
		} else {
			fprintf(stderr, "!!!!!! %s not yet implemented for Program !!!!!\n", columnName);
		}
	}
	sqlite3_finalize(stmt);

	return program;
}

