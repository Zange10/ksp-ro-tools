#ifndef KSP_MISSION_DATABASE_H
#define KSP_MISSION_DATABASE_H


enum MissionStatus_DB {YET_TO_FLY, IN_PROGRESS, ENDED};
enum MissionSuccess {MISSION_SUCCESS, MISSION_PARTIAL_SUCCESS, MISSION_FAIL, MISSION_SUCCESS_TBD};

struct Mission_DB {
	int id;
	char name[30];
	int program_id;
	enum MissionStatus_DB status;
	int launcher_id;
	int plane_id;
};

struct Mission_Filter {
	char name[30];
	int program_id;
	int ytf, in_prog, ended;
};

struct MissionProgram_DB {
	int id;
	char name[30];
	char vision[255];
};

void db_new_program(const char *program_name, const char *vision);
void db_new_mission(const char *mission_name, int program_id, int launcher_id, int status);
void db_update_mission(int mission_id, const char *mission_name, int program_id, int launcher_id, int status);
int db_get_number_of_programs();
int db_get_missions_ordered_by_launch_date(struct Mission_DB **p_missions, struct Mission_Filter filter);
int db_get_all_programs(struct MissionProgram_DB **p_programs);
struct MissionProgram_DB db_get_program_from_id(int id);
enum MissionSuccess db_get_mission_success(int mission_id);

#endif //KSP_MISSION_DATABASE_H
