#include "transfer_calculator.h"
#include "transfer_tools.h"
#include "tools/csv_writer.h"
#include "tools/tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "tools/thread_pool.h"
#include "database/itin_db.h"

void print_step(struct ItinStep *step) {
	printf("%s\n", step->body->name);
	print_date(convert_JD_date(step->date),1);
	print_vector(scalar_multiply(step->r,1e-9));
	print_vector(scalar_multiply(step->v_dep,1e-3));
	print_vector(scalar_multiply(step->v_arr,1e-3));
	print_vector(scalar_multiply(step->v_body,1e-3));
	printf("Num Nodes: %d\n", step->num_next_nodes);
}

void print_itinerary(struct ItinStep *itin) {
	if(itin->prev != NULL) {
		print_itinerary(itin->prev);
		printf(" - ");
	}
	print_date(convert_JD_date(itin->date), 0);
}

void print_itinerary2(struct ItinStep *itin, int step) {
	if(itin->prev != NULL) printf("|");
	else printf("\n");
	for(int i = 0; i < step; i++) printf("-");
	printf("%d\n", itin->num_next_nodes);
	if(itin->num_next_nodes == 0) {
		return;
	}

	for(int i = 0; i < itin->num_next_nodes; i++) {
		print_itinerary2(itin->next[i], step+1);
	}
}

int get_number_of_itineraries(struct ItinStep *itin) {
	if(itin->num_next_nodes == 0) return 1;
	int counter = 0;
	for(int i = 0; i < itin->num_next_nodes; i++) {
		counter += get_number_of_itineraries(itin->next[i]);
	}
	return counter;
}

int get_total_number_of_stored_steps(struct ItinStep *itin) {
	if(itin->num_next_nodes == 0) return 1;
	int counter = 1;
	for(int i = 0; i < itin->num_next_nodes; i++) {
		counter += get_total_number_of_stored_steps(itin->next[i]);
	}
	return counter;
}

void store_itineraries_in_array(struct ItinStep *itin, struct ItinStep **array, int *index) {
	if(itin->num_next_nodes == 0) {
		array[*index] = itin;
		(*index)++;
	}

	for(int i = 0; i < itin->num_next_nodes; i++) {
		store_itineraries_in_array(itin->next[i], array, index);
	}
}

double get_itinerary_duration(struct ItinStep *itin) {
	double jd1 = itin->date;
	while(itin->prev != NULL) itin = itin->prev;
	double jd0 = itin->date;
	return jd1-jd0;
}

void create_porkchop_point(struct ItinStep *itin, double* porkchop, int circ_cap_fb) {
	double dv, vinf = vector_mag(add_vectors(itin->v_arr, scalar_multiply(itin->v_body,-1)));
	if(circ_cap_fb == 0) dv = dv_circ(itin->body, itin->body->atmo_alt+100e3, vinf);
	else if(circ_cap_fb == 1) dv = dv_capture(itin->body, itin->body->atmo_alt+100e3, vinf);
	else dv = 0;
	porkchop[4] = dv;
	porkchop[1] = get_itinerary_duration(itin);

	porkchop[3] = 0;

	while(itin->prev->prev != NULL) {
		if(itin->body == NULL) {
			porkchop[3] += vector_mag(add_vectors(itin->next[0]->v_dep, scalar_multiply(itin->v_arr,-1)));
		}
		itin = itin->prev;
	}

	vinf = vector_mag(add_vectors(itin->v_dep, scalar_multiply(itin->prev->v_body,-1)));
	porkchop[2] = dv_circ(itin->prev->body, itin->prev->body->atmo_alt+100e3, vinf);
	porkchop[0] = itin->prev->date;
}

void remove_step_from_itinerary(struct ItinStep *step) {
	struct ItinStep *prev = step->prev;

	while(prev->num_next_nodes == 1 && step->prev->prev != NULL) {
		step = prev;
		prev = step->prev;
	}

	int index = 0;
	for(int i = 0; i < prev->num_next_nodes; i++) {
		if(prev->next[i] == step) { index = i; break; }
	}
	prev->num_next_nodes--;
	for(int i = index; i < prev->num_next_nodes; i++) {
		prev->next[i] = prev->next[i+1];
	}
	free_itinerary(step);
}

int calc_next_step(struct ItinStep *curr_step, struct Ephem **ephems, struct Body **bodies, const int *min_duration, const int *max_duration, int num_steps, int step) {
	if(bodies[step] != bodies[step-1]) find_viable_flybys(curr_step, ephems, bodies[step], min_duration[step-1]*86400, max_duration[step-1]*86400);
	else {
		find_viable_dsb_flybys(curr_step, ephems, bodies[step+1],
							   min_duration[step-1]*86400, max_duration[step-1]*86400, min_duration[step]*86400, max_duration[step]*86400);
	}

	if(curr_step->num_next_nodes == 0) {
		remove_step_from_itinerary(curr_step);
		return 0;
	}

	int num_valid = 0;
	int init_num_nodes = curr_step->num_next_nodes;
	int step_del = 0;
	int result;

	if(step < num_steps-1) {
		for(int i = 0; i < init_num_nodes; i++) {
			if(bodies[step] != bodies[step-1]) result = calc_next_step(curr_step->next[i-step_del], ephems, bodies, min_duration, max_duration, num_steps, step+1);
			else result = calc_next_step(curr_step->next[i-step_del]->next[0]->next[0], ephems, bodies, min_duration, max_duration, num_steps, step+2);
			num_valid += result;
			if(!result) step_del++;
		}
	} else return 1;

	return num_valid > 0;
}

void store_itineraries_in_file(struct ItinStep *step, FILE *file, int layer, int variation) {
	fprintf(file, "#%d#%d\n", layer, variation);
	fprintf(file, "Date: %f\n", step->date);
	fprintf(file, "r: %lf, %lf, %lf\n", step->r.x, step->r.y, step->r.z);
	fprintf(file, "v_dep: %f, %f, %f\n", step->v_dep.x, step->v_dep.y, step->v_dep.z);
	fprintf(file, "v_arr: %f, %f, %f\n", step->v_arr.x, step->v_arr.y, step->v_arr.z);
	fprintf(file, "v_body: %f, %f, %f\n", step->v_body.x, step->v_body.y, step->v_body.z);
	fprintf(file, "Next Steps: %d\n", step->num_next_nodes);

	for(int i = 0; i < step->num_next_nodes; i++) {
		store_itineraries_in_file(step->next[i], file, layer+1, i);
	}
}

void store_itineraries_in_file_init(struct ItinStep **departures, int num_nodes, int num_deps, int num_steps) {
	char filename[19];  // 14 for date + 4 for .csv + 1 for string terminator
	sprintf(filename, "test.transfer");

	printf("Filesize: ~%f.3 MB\n", (double)num_nodes*240/1e6);

	FILE *file;
	file = fopen(filename,"w");

	fprintf(file, "Number of stored nodes: %d\n", num_nodes);
	fprintf(file, "Number of Departures: %d, Number of Steps: %d\n", num_deps, num_steps);

	struct ItinStep *ptr = departures[0];

	fprintf(file, "Bodies: ");
	while(ptr != NULL) {
		if(ptr->body != NULL) fprintf(file, "%d", ptr->body->id);
		else fprintf(file, "-");
		if(ptr->next != NULL) {
			fprintf(file, ",");
			ptr = ptr->next[0];
		} else {
			fprintf(file, "#\n");
			break;
		}
	}

	for(int i = 0; i < num_deps; i++) {
		store_itineraries_in_file(departures[i], file, 0, i);
	}

	fclose(file);
}

struct Itin_Thread_Args {
	double jd_min_dep;
	double jd_max_dep;
	struct ItinStep **departures;
	struct Ephem **ephems;
	struct Body **bodies;
	int *min_duration;
	int *max_duration;
	int num_steps;
};

void *calc_from_departure(void *args) {
	struct Itin_Thread_Args *thread_args = (struct Itin_Thread_Args *)args;

	int *min_duration = thread_args->min_duration;
	int *max_duration = thread_args->max_duration;
	struct Ephem **ephems = thread_args->ephems;
	struct Body **bodies = thread_args->bodies;
	int num_steps = thread_args->num_steps;

	int index = get_thread_counter();
	double jd_dep = thread_args->jd_min_dep + index;
	struct ItinStep *curr_step;

	double jd_diff = thread_args->jd_max_dep-thread_args->jd_min_dep+1;

	while(jd_dep <= thread_args->jd_max_dep) {
		struct OSV osv_body0 = osv_from_ephem(ephems[bodies[0]->id - 1], jd_dep, SUN());

		curr_step = thread_args->departures[index];
		curr_step->body = bodies[0];
		curr_step->date = jd_dep;
		curr_step->r = osv_body0.r;
		curr_step->v_body = osv_body0.v;
		curr_step->v_dep = vec(0, 0, 0);
		curr_step->v_arr = vec(0, 0, 0);
		curr_step->num_next_nodes = max_duration[0] - min_duration[0] + 1;
		curr_step->prev = NULL;
		curr_step->next = (struct ItinStep **) malloc(
				(max_duration[0] - min_duration[0] + 1) * sizeof(struct ItinStep *));

		int fb1_del = 0;

		for(int j = 0; j <= max_duration[0] - min_duration[0]; j++) {
			double jd_arr = jd_dep + min_duration[0] + j;
			struct OSV osv_body1 = osv_from_ephem(ephems[bodies[1]->id - 1], jd_arr, SUN());

			struct Transfer tf = calc_transfer(circfb, bodies[0], bodies[1], osv_body0.r, osv_body0.v,
											   osv_body1.r, osv_body1.v, (jd_arr - jd_dep) * 86400, NULL);

			while(curr_step->prev != NULL) curr_step = curr_step->prev;
			curr_step->next[j - fb1_del] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			curr_step->next[j - fb1_del]->prev = curr_step;
			curr_step = curr_step->next[j - fb1_del];
			curr_step->body = bodies[1];
			curr_step->date = jd_arr;
			curr_step->r = osv_body1.r;
			curr_step->v_dep = tf.v0;
			curr_step->v_arr = tf.v1;
			curr_step->v_body = osv_body1.v;
			curr_step->num_next_nodes = 0;
			curr_step->next = NULL;

			if(num_steps > 2) {
				if(!calc_next_step(curr_step, ephems, bodies, min_duration, max_duration, num_steps, 2)) {
					fb1_del++;
				}
			}
		}
		show_progress("Transfer Calculation progress: ", index, jd_diff);
		index = get_thread_counter();
		jd_dep = thread_args->jd_min_dep + index;
	}
	return NULL;
}

void create_itinerary() {
	struct timeval start, end;
	double elapsed_time;
	gettimeofday(&start, NULL);  // Record the ending time

	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}

//	struct Body *bodies[] = {EARTH(), VENUS(), MARS(), EARTH()};
//	int num_steps = sizeof(bodies)/sizeof(struct Body*);
//
//	struct Date min_dep_date = {1959, 6, 15};
//	struct Date max_dep_date = {1959, 9, 31};
//	double jd_min_dep = convert_date_JD(min_dep_date);
//	double jd_max_dep = convert_date_JD(max_dep_date);
//	int num_deps = (int) (jd_max_dep-jd_min_dep+1);
//
//	int min_duration[] = {50, 100, 50};
//	int max_duration[] = {250, 500, 500};

	struct Body *bodies[] = {EARTH(), JUPITER(), SATURN(), URANUS(), NEPTUNE()};
	int num_steps = sizeof(bodies)/sizeof(struct Body*);

	struct Date min_dep_date = {1975, 5, 15};
	struct Date max_dep_date = {1979, 11, 21};
	double jd_min_dep = convert_date_JD(min_dep_date);
	double jd_max_dep = convert_date_JD(max_dep_date);
	int num_deps = (int) (jd_max_dep-jd_min_dep+1);

	int min_duration[] = {500, 100, 300, 300};
	int max_duration[] = {1000, 3000, 3000, 3000};

//	struct Body *bodies[] = {EARTH(), VENUS(), VENUS(), EARTH(), JUPITER(), SATURN()};
//	int num_steps = sizeof(bodies)/sizeof(struct Body*);
//
//	struct Date min_dep_date = {1997, 10, 1};
//	struct Date max_dep_date = {1997, 10, 31};
//	double jd_min_dep = convert_date_JD(min_dep_date);
//	double jd_max_dep = convert_date_JD(max_dep_date);
//	int num_deps = (int) (jd_max_dep-jd_min_dep+1);
//
//	int min_duration[] = {195, 422, 50, 200, 500};
//	int max_duration[] = {196, 428, 60, 1000, 2000};

	struct ItinStep **departures = (struct ItinStep**) malloc(num_deps * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) departures[i] = (struct ItinStep*) malloc(sizeof(struct ItinStep));

	struct Itin_Thread_Args thread_args = {
			jd_min_dep,
			jd_max_dep,
			departures,
			ephems,
			bodies,
			min_duration,
			max_duration,
			num_steps
	};

	struct Thread_Pool thread_pool = use_thread_pool64(calc_from_departure, &thread_args);
	join_thread_pool(thread_pool);
	show_progress("Transfer Calculation progress: ", 1, 1);
	printf("\n");

	// remove departure dates with no valid itinerary
	for(int i = 0; i < num_deps; i++) {
		if(departures[i] == NULL || departures[i]->num_next_nodes == 0) {
			num_deps--;
			for(int j = i; j < num_deps; j++) {
				departures[j] = departures[j + 1];
			}
			i--;
		}
	}


//	for(int i = 0; i < num_deps; i++) {
//		print_itinerary2(departures[i], 0);
//	}
	int num_itins = 0, tot_num_itins = 0;
	for(int i = 0; i < num_deps; i++) num_itins += get_number_of_itineraries(departures[i]);
	for(int i = 0; i < num_deps; i++) tot_num_itins += get_total_number_of_stored_steps(departures[i]);


	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);

	if(num_itins == 0) {
		printf("\nNo itineraries found!\n");
		for(int i = 0; i < num_deps; i++) free_itinerary(departures[i]);
		free(departures);
		for(int i = 0; i < num_bodies; i++) free(ephems[i]);
		free(ephems);
		return;
	} else printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins, tot_num_itins);


	int index = 0;
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(departures[i], arrivals, &index);

	double *porkchop = (double *) malloc((5 * num_itins + 1) * sizeof(double));
	porkchop[0] = 0;
	for(int i = 0; i < num_itins; i++) {
		create_porkchop_point(arrivals[i], &porkchop[i*5+1], -1);
		porkchop[0] += 5;
	}

//	for(int i = 0; i < num_itins; i++) {
//		print_itinerary(arrivals[i]);
//		printf("\n");
//	}

	char data_fields[] = "dep_date,duration,dv_dep,dv_mcc,dv_arr";
	write_csv(data_fields, porkchop);

	double mindv = porkchop[1+2]+porkchop[1+3]+porkchop[1+4];
	int mind = 0;
	for(int i = 1; i < num_itins; i++) {
		double dv = porkchop[1+i*5+2]+porkchop[1+i*5+3]+porkchop[1+i*5+4];
		if(dv < mindv) {
			mindv = dv;
			mind = i;
		}
	}

	print_itinerary(arrivals[mind]);
	printf("  -  %f m/s\n", mindv);

	store_itineraries_in_file_init(departures, tot_num_itins, num_deps, num_steps);

	printf("file done\n");

	store_itineraries_in_db(departures, num_deps, "Inner Voyage");


	for(int i = 0; i < num_deps; i++) free_itinerary(departures[i]);
	free(departures);
	free(arrivals);
	free(porkchop);
	for(int i = 0; i < num_bodies; i++) free(ephems[i]);
	free(ephems);
}
