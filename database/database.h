#ifndef KSP_DATABASE_H
#define KSP_DATABASE_H

#include "sqlite3.h"

enum MissionStatus {YET_TO_FLY, IN_PROGRESS, ENDED};
enum VehicleStatus {ACTIVE, INACTIVE};

#define MISSION_COLS 5	// one less than table has columns because plane and lv get in one

struct Mission {
	int id;
	char name[30];
	int program_id;
	enum MissionStatus status;
	int launcher_id;
	int plane_id;
};

struct LaunchVehicle {
	int id;
	char name[30];
	int family_id;
	double leo, sso, gto, tli, tvi;
	double payload_diameter, A, c_d;
	enum VehicleStatus status;
};

// Mathematical Planes and Aeroplanes have for some reason the same name...
struct FlyVehicle {
	int id;
	char name[30];
	int rated_alt;
	double F_vac;
	double F_sl;
	double burnrate;
	double mass_dry;
	double mass_wet;
	enum VehicleStatus status;
};

struct MissionProgram {
	int id;
	char name[30];
	char vision[255];
};


void init_db();
int execute_query(sqlite3 *query_db, const char *query);
int db_get_missions(struct Mission **missions);
struct LaunchVehicle db_get_lv_from_id(int id);
struct FlyVehicle db_get_plane_from_id(int id);
struct MissionProgram db_get_program_from_id(int id);
void close_db();

#endif //KSP_DATABASE_H
