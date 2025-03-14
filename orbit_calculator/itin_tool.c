#include "itin_tool.h"
#include "orbit.h"
#include "transfer_tools.h"
#include "tools/data_tool.h"
#include "double_swing_by.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>



void find_viable_flybys(struct ItinStep *tf, struct System *system, struct Body *next_body, double min_dt, double max_dt) {
	struct OSV osv_dep = {tf->r, tf->v_body};
	struct OSV osv_arr0 = system->calc_method == ORB_ELEMENTS ?
			osv_from_elements(next_body->orbit, tf->date, system->cb) :
			osv_from_ephem(next_body->ephem, tf->date, system->cb);

	struct Vector proj_vec = proj_vec_plane(osv_dep.r, constr_plane(vec(0,0,0), osv_arr0.r, osv_arr0.v));
	double theta_conj_opp = angle_vec_vec(proj_vec, osv_arr0.r);
	if(cross_product(proj_vec, osv_arr0.r).z < 0) theta_conj_opp *= -1;
	else theta_conj_opp -= M_PI;

	int max_new_steps = 100;
	struct ItinStep *new_steps[max_new_steps];

	int counter = 0;

	struct OSV osv_arr1 = propagate_orbit_theta(constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, system->cb), -theta_conj_opp, SUN());
	struct Orbit arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, system->cb);
	struct Orbit arr1 = constr_orbit_from_osv(osv_arr1.r, osv_arr1.v, system->cb);
	double dt0 = arr1.t-arr0.t;

	osv_arr1 = propagate_orbit_theta(constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, system->cb), -theta_conj_opp+M_PI, system->cb);
	arr0 = constr_orbit_from_osv(osv_arr0.r, osv_arr0.v, system->cb);
	arr1 = constr_orbit_from_osv(osv_arr1.r, osv_arr1.v, system->cb);
	double dt1 = arr1.t-arr0.t;

	while(dt0 < 0) dt0 += arr0.period;
	while(dt1 < 0) dt1 += arr0.period;
	if(dt0 < dt1) {
		double temp = dt0;
		dt0 = dt1-arr0.period;
		dt1 = temp;
	} else {
		dt0 -= arr0.period;
	}

	while(dt1 < min_dt) {
		double temp = dt1;
		dt1 = dt0 + arr0.period;
		dt0 = temp;
	}

	// x: dt, y: diff_vinf (data[0].x: number of data points beginning at index 1)
	struct Vector2D data[101];

	double t0 = tf->date;
	double last_dt, dt, t1, diff_vinf;

	struct Vector v_init = subtract_vectors(tf->v_arr, tf->v_body);

	while(dt0 < max_dt && counter < max_new_steps) {
		data[0].x = 0;
		int right_side = 0;	// 0 = left, 1 = right

		for(int i = 0; i < 100; i++) {
			if(i == 0) dt = dt0;
			if(dt < min_dt) dt = min_dt;
			if(dt > max_dt) dt = max_dt;

			t1 = t0 + dt / 86400;

			struct OSV osv_arr = system->calc_method == ORB_ELEMENTS ?
					osv_from_elements(next_body->orbit, t1, system->cb) :
					osv_from_ephem(next_body->ephem, t1, system->cb);

			struct Transfer new_transfer = calc_transfer(circfb, tf->body, next_body, osv_dep.r, osv_dep.v, osv_arr.r, osv_arr.v, dt,
														 system->cb, NULL);

			struct Vector v_dep = subtract_vectors(new_transfer.v0, tf->v_body);

			diff_vinf = vector_mag(v_dep) - vector_mag(v_init);

			if (fabs(diff_vinf) < 1) {
				double beta = (M_PI - angle_vec_vec(v_dep, v_init)) / 2;
				double rp = (1 / cos(beta) - 1) * (tf->body->mu / (pow(vector_mag(v_dep), 2)));

				if (rp > tf->body->radius + tf->body->atmo_alt) {
					new_steps[counter] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
					new_steps[counter]->body = next_body;
					new_steps[counter]->date = t1;
					new_steps[counter]->r = osv_arr.r;
					new_steps[counter]->v_dep = new_transfer.v0;
					new_steps[counter]->v_arr = new_transfer.v1;
					new_steps[counter]->v_body = osv_arr.v;
					new_steps[counter]->num_next_nodes = 0;
					new_steps[counter]->prev = tf;
					new_steps[counter]->next = NULL;
					counter++;
				}

				if(!right_side) right_side = 1;
				else break;
			}


			insert_new_data_point(data, dt, diff_vinf);

			if(!can_be_negative_monot_deriv(data)) break;
			last_dt = dt;
			if(i == 0) dt = dt1;
			else dt = root_finder_monot_deriv_next_x(data, right_side ? 1 : 0);
			if(i > 3) {
				if(dt == last_dt) break;
				if(dt >= dt1) dt = (dt1+data[(int) data[0].x-1].x)/2;
				if(dt < dt0) dt = (dt0+data[2].x)/2;
			}
			if(isnan(dt) || isinf(dt)) break;
		}



		double temp = dt1;
		dt1 = dt0 + arr0.period;
		dt0 = temp;
	}

	if(counter > 0) {
		if(tf->num_next_nodes == 0) tf->next = (struct ItinStep **) malloc(counter * sizeof(struct ItinStep *));
		else {
			struct ItinStep ** new_next = realloc(tf->next, (counter+tf->num_next_nodes) * sizeof(struct ItinStep *));
			if(new_next == NULL) {
				printf("\nERROR WHILE REALLOCATING ITINERARIES DURING TRANSFER CALCULATION!!!\n");
				return;
			}
			tf->next = new_next;
		}
		for(int i = 0; i < counter; i++) tf->next[i+tf->num_next_nodes] = new_steps[i];

		tf->num_next_nodes += counter;
	}
}


void find_viable_dsb_flybys(struct ItinStep *tf, struct Ephem **ephems, struct Body *body1, double min_dt0, double max_dt0, double min_dt1, double max_dt1) {
	int max_new_steps0 = (int) (max_dt0/86400-min_dt0/86400+1);
	int max_new_steps1 = (int) (max_dt1/86400-min_dt1/86400+1);
	struct ItinStep *new_steps[max_new_steps0*max_new_steps1];
	struct Body *body0 = tf->body;

	int counter = 0;

	struct OSV s0 = {tf->r, tf->v_arr};
	struct OSV p0 = {tf->r, tf->v_body};
	struct OSV s1;
	double dt0, dt1, jd_sb2, jd_arr;


	for(int i = 0; i <= max_dt0/86400-min_dt0/86400; i++) {

		dt0 = min_dt0/86400 + i;
		jd_sb2 = tf->date+dt0;
		struct OSV osv_sb2 = osv_from_ephem(ephems[0], jd_sb2, SUN());

		for(int j = 0; j <= max_dt1/86400-min_dt1/86400; j++) {
			dt1 = min_dt1/86400 + j;
			jd_arr = jd_sb2 + dt1;

			struct OSV osv_arr = osv_from_ephem(ephems[1], jd_arr, SUN());
			struct Transfer transfer_after_dsb = calc_transfer(circfb, body0, body1, osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, SUN(), NULL);

			s1.r = transfer_after_dsb.r0;
			s1.v = transfer_after_dsb.v0;

			struct DSB dsb = calc_double_swing_by(s0, p0, s1, osv_sb2, dt0, body0);

			if(dsb.dv < 10000) {
				new_steps[counter] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
				new_steps[counter]->body = NULL;
				new_steps[counter]->date = tf->date + dsb.man_time/86400;
				new_steps[counter]->r = dsb.osv[1].r;
				new_steps[counter]->v_dep = dsb.osv[0].v;
				new_steps[counter]->v_arr = dsb.osv[1].v;
				new_steps[counter]->v_body = vec(0,0,0);
				new_steps[counter]->num_next_nodes = 1;
				new_steps[counter]->prev = tf;
				new_steps[counter]->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
				new_steps[counter]->next[0] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
				new_steps[counter]->next[0]->prev = new_steps[counter];

				struct ItinStep *curr = new_steps[counter]->next[0];
				curr->body = body0;
				curr->date = jd_sb2;
				curr->r = osv_sb2.r;
				curr->v_dep = dsb.osv[2].v;
				curr->v_arr = dsb.osv[3].v;
				curr->v_body = osv_sb2.v;
				curr->num_next_nodes = 1;
				curr->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
				curr->next[0] = (struct ItinStep*) malloc(sizeof(struct ItinStep));
				curr->next[0]->prev = curr;

				curr = curr->next[0];
				curr->body = body1;
				curr->date = jd_arr;
				curr->r = osv_arr.r;
				curr->v_dep = transfer_after_dsb.v0;
				curr->v_arr = transfer_after_dsb.v1;
				curr->v_body = osv_arr.v;
				curr->num_next_nodes = 0;
				curr->next = NULL;
				counter++;
			}
		}
	}

	// if there were next nodes found, store them
	if(counter > 0) {
		tf->next = (struct ItinStep **) malloc(counter * sizeof(struct ItinStep *));
		for(int i = 0; i < counter; i++) tf->next[i] = new_steps[i];
		tf->num_next_nodes = counter;
	}
}



struct ItinStep * get_first(struct ItinStep *tf) {
	if(tf == NULL) return NULL;
	while(tf->prev != NULL) tf = tf->prev;
	return tf;
}

struct ItinStep * get_last(struct ItinStep *tf) {
	if(tf == NULL) return NULL;
	while(tf->next != NULL) tf = tf->next[0];
	return tf;
}


void print_itinerary(struct ItinStep *itin) {
	if(itin->prev != NULL) {
		print_itinerary(itin->prev);
		printf(" - ");
	}
	print_date(convert_JD_date(itin->date), 0);
}

int get_number_of_itineraries(struct ItinStep *itin) {
	if(itin == NULL) return 0;
	if(itin->num_next_nodes == 0) return 1;
	int counter = 0;
	for(int i = 0; i < itin->num_next_nodes; i++) {
		counter += get_number_of_itineraries(itin->next[i]);
	}
	return counter;
}

int get_total_number_of_stored_steps(struct ItinStep *itin) {
	if(itin == NULL) return 0;
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

void create_porkchop_point(struct ItinStep *itin, double* porkchop, int circ0_cap1) {
	double vinf = vector_mag(subtract_vectors(itin->v_arr, itin->v_body));

	porkchop[4] = circ0_cap1 == 0 ? dv_circ(itin->body, itin->body->atmo_alt+100e3, vinf) : dv_capture(itin->body, itin->body->atmo_alt+100e3, vinf);
	porkchop[1] = get_itinerary_duration(itin);

	porkchop[3] = 0;

	while(itin->prev->prev != NULL) {
		if(itin->body == NULL) {
			porkchop[3] += vector_mag(subtract_vectors(itin->next[0]->v_dep, itin->v_arr));
		}
		itin = itin->prev;
	}

	vinf = vector_mag(subtract_vectors(itin->v_dep, itin->prev->v_body));
	porkchop[2] = dv_circ(itin->prev->body, itin->prev->body->atmo_alt+100e3, vinf);
	porkchop[0] = itin->prev->date;
}

int calc_next_spec_itin_step(struct ItinStep *curr_step, struct System *system, struct Body **bodies, const int *min_duration, const int *max_duration, struct Dv_Filter *dv_filter, int num_steps, int step) {
	if(bodies[step] != bodies[step-1]) find_viable_flybys(curr_step, system, bodies[step], min_duration[step-1]*86400, max_duration[step-1]*86400);
	else {
		find_viable_dsb_flybys(curr_step, &bodies[step+1]->ephem, bodies[step+1],
							   min_duration[step-1]*86400, max_duration[step-1]*86400, min_duration[step]*86400, max_duration[step]*86400);
	}

	for(int i = 0; i < curr_step->num_next_nodes; i++) {
		struct ItinStep *next = bodies[step] != bodies[step-1] ? curr_step->next[i] : curr_step->next[i]->next[0]->next[0];
		double porkchop[5];
		create_porkchop_point(next, porkchop, dv_filter->last_transfer_type == 1);
		if(step == num_steps-1) {
			int fb0_pow1 = dv_filter->last_transfer_type != 0;
			if(porkchop[3] + porkchop[4] * fb0_pow1 > dv_filter->max_satdv ||
					porkchop[2] + porkchop[3] + porkchop[4] * fb0_pow1 > dv_filter->max_totdv) {
				if(curr_step->num_next_nodes <= 1) {
					remove_step_from_itinerary(next);
					return 0;
				} else {
					remove_step_from_itinerary(next);
				}
				i--;
			}
		}
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
			if(bodies[step] != bodies[step-1]) result = calc_next_spec_itin_step(curr_step->next[i - step_del], system, bodies, min_duration, max_duration, dv_filter, num_steps, step + 1);
			else result = calc_next_spec_itin_step(curr_step->next[i - step_del]->next[0]->next[0], system, bodies, min_duration, max_duration, dv_filter, num_steps, step + 2);
			num_valid += result;
			if(!result) step_del++;
		}
	} else return 1;

	return num_valid > 0;
}

int calc_next_itin_to_target_step(struct ItinStep *curr_step, struct System *system, struct Body *arr_body, double jd_max_arr, double max_total_duration, struct Dv_Filter *dv_filter) {
	for(int body_id = 0; body_id < system->num_bodies; body_id++) {
		if(system->bodies[body_id] == curr_step->body) continue;
		double jd_max;
		if(get_first(curr_step)->date + max_total_duration < jd_max_arr)
			jd_max = get_first(curr_step)->date + max_total_duration;
		else
			jd_max = jd_max_arr;
		double max_duration = jd_max-curr_step->date;
		double min_duration = 10;	// [days]
		if(max_duration > min_duration)
			find_viable_flybys(curr_step, system, system->bodies[body_id], min_duration*86400, max_duration*86400);
	}


	if(curr_step->num_next_nodes == 0) {
		remove_step_from_itinerary(curr_step);
		return 0;
	}

	int num_of_end_nodes = find_copy_and_store_end_nodes(curr_step, arr_body);

	// remove end nodes that do not satisfy dv requirements
	num_of_end_nodes = remove_end_nodes_that_do_not_satisfy_dv_requirements(curr_step, num_of_end_nodes, dv_filter);
	if(curr_step->num_next_nodes <= 0) return 0;

	// returns 1 if has valid steps (0 otherwise)
	return continue_to_next_steps_and_check_for_valid_itins(curr_step, num_of_end_nodes, system, arr_body, jd_max_arr, max_total_duration, dv_filter);
}

int continue_to_next_steps_and_check_for_valid_itins(struct ItinStep *curr_step, int num_of_end_nodes, struct System *system, struct Body *arr_body, double jd_max_arr, double max_total_duration, struct Dv_Filter *dv_filter) {
	int num_valid = num_of_end_nodes;
	int init_num_nodes = curr_step->num_next_nodes;
	int step_del = 0;
	int has_valid_init;

	for(int i = 0; i < init_num_nodes-num_of_end_nodes; i++) {
		int idx = i - step_del;
		has_valid_init = calc_next_itin_to_target_step(curr_step->next[idx], system, arr_body, jd_max_arr, max_total_duration, dv_filter);
		num_valid += has_valid_init;
		if(!has_valid_init) step_del++;
	}

	return num_valid > 0;
}

int find_copy_and_store_end_nodes(struct ItinStep *curr_step, struct Body *arr_body) {
	int num_of_end_nodes = 0;
	for(int i = 0; i < curr_step->num_next_nodes; i++) {
		if(curr_step->next[i]->body == arr_body) num_of_end_nodes++;
	}
	// if there were end nodes found, copy them and store them at the end
	if(num_of_end_nodes > 0) {
		struct ItinStep ** new_next = realloc(curr_step->next, (num_of_end_nodes+curr_step->num_next_nodes) * sizeof(struct ItinStep *));
		if(new_next == NULL) {
			printf("\nERROR WHILE REALLOCATING ITINERARIES DURING END NODE COPYING!!!\n");
			return 0;
		}
		curr_step->next = new_next;

		int end_node_idx = 0;

		for(int i = 0; i < curr_step->num_next_nodes; i++) {
			if(curr_step->next[i]->body == arr_body) {
				struct ItinStep *end_node = malloc(num_of_end_nodes * sizeof(struct ItinStep));
				end_node->body 	= curr_step->next[i]->body;
				end_node->date 	= curr_step->next[i]->date;
				end_node->r 	= curr_step->next[i]->r;
				end_node->v_dep = curr_step->next[i]->v_dep;
				end_node->v_arr = curr_step->next[i]->v_arr;
				end_node->v_body= curr_step->next[i]->v_body;
				end_node->num_next_nodes = 0;
				end_node->prev 	= curr_step->next[i]->prev;
				end_node->next 	= NULL;

				curr_step->next[curr_step->num_next_nodes + end_node_idx] = end_node;
				end_node_idx++;
			}
		}
	}

	curr_step->num_next_nodes += num_of_end_nodes;
	return num_of_end_nodes;
}

int remove_end_nodes_that_do_not_satisfy_dv_requirements(struct ItinStep *curr_step, int num_of_end_nodes, struct Dv_Filter *dv_filter) {
	for(int i = curr_step->num_next_nodes-num_of_end_nodes; i < curr_step->num_next_nodes; i++) {
		struct ItinStep *next = curr_step->next[i];
		double porkchop[5];
		create_porkchop_point(next, porkchop, dv_filter->last_transfer_type == 1);
		int fb0_pow1 = dv_filter->last_transfer_type != 0;
		if(porkchop[3] + porkchop[4] * fb0_pow1 > dv_filter->max_satdv ||
		   porkchop[2] + porkchop[3] + porkchop[4] * fb0_pow1 > dv_filter->max_totdv) {
			if(curr_step->num_next_nodes <= 1) {
				remove_step_from_itinerary(next);
				curr_step->num_next_nodes = 0;
			} else {
				remove_step_from_itinerary(next);
			}
			i--;
			num_of_end_nodes--;
		}
	}
	return num_of_end_nodes;
}

int get_num_of_itin_layers(struct ItinStep *step) {
	int counter = 0;
	while(step != NULL) {
		counter++;
		if(step->next != NULL) step = step->next[0];
		else break;
	}
	return counter;
}

void update_itin_body_osvs(struct ItinStep *step, struct System *system) {
	struct OSV body_osv;
	while(step != NULL) {
		if(step->body != NULL) {
			body_osv = system->calc_method == ORB_ELEMENTS ?
					osv_from_elements(step->body->orbit, step->date, system->cb) :
					osv_from_ephem(step->body->ephem, step->date, system->cb);
			step->r = body_osv.r;
			step->v_body = body_osv.v;
		} else step->v_body = vec(0, 0, 0);	// don't draw the trajectory (except changed in later calc)
		step = step->next != NULL ? step->next[0] : NULL;
	}
}

void calc_itin_v_vectors_from_dates_and_r(struct ItinStep *step, struct System *system) {
	if(step == NULL) return;
	struct ItinStep *next;
	while(step->next != NULL) {
		next = step->next[0];

		if(next->body == NULL) {
			if(next->next == NULL) {
				step = next;
				continue;
			} else if(next->next[0]->next == NULL) {
				next = next->next[0];
				double dt = (next->date - step->date) * 86400;
				struct Transfer transfer = calc_transfer(circcap, step->body, next->body, step->r, step->v_body, next->r,
														 next->v_body, dt, system->cb, NULL);
				next->v_dep = transfer.v0;
				next->v_arr = transfer.v1;
			} else {
				struct ItinStep *sb2 = next->next[0];
				struct ItinStep *arr = next->next[0]->next[0];
				struct OSV osv_sb2 = {next->next[0]->r, next->next[0]->v_body};
				struct OSV osv_arr = {next->next[0]->next[0]->r, next->next[0]->next[0]->v_body};
				struct Transfer transfer_after_dsb = calc_transfer(circfb, sb2->body, arr->body, osv_sb2.r, osv_sb2.v,
																   osv_arr.r, osv_arr.v,
																   (arr->date - sb2->date) * 86400, system->cb, NULL);

				struct OSV s0 = {step->r, step->v_arr};
				struct OSV p0 = {step->r, step->v_body};
				struct OSV s1 = {transfer_after_dsb.r0, transfer_after_dsb.v0};

				double dt = sb2->date - step->date;
				struct DSB dsb = calc_double_swing_by(s0, p0, s1, osv_sb2, dt, sb2->body);
				if(dsb.dv < 10000) {
					next->r = dsb.osv[1].r;
					next->v_dep = dsb.osv[0].v;
					next->v_arr = dsb.osv[1].v;
					next->date = step->date + dsb.man_time / 86400;
					next->v_body = vec(1, 0, 0);    // draw the trajectory
				} else {
					next = next->next[0];
					dt = (next->date - step->date) * 86400;
					struct Transfer transfer = calc_transfer(circcap, step->body, next->body, step->r, step->v_body, next->r,
															 next->v_body, dt, system->cb, NULL);
					next->v_dep = transfer.v0;
					next->v_arr = transfer.v1;
				}
			}
		} else {
			double dt = (next->date - step->date) * 86400;
			struct Transfer transfer = calc_transfer(circcap, step->body, next->body, step->r, step->v_body, next->r,
													 next->v_body, dt, system->cb, NULL);
			next->v_dep = transfer.v0;
			next->v_arr = transfer.v1;
		}
		step = next;
	}
}

void copy_step_body_vectors_and_date(struct ItinStep *orig_step, struct ItinStep *step_copy) {
	step_copy->body = orig_step->body;
	step_copy->r = orig_step->r;
	step_copy->v_body = orig_step->v_body;
	step_copy->v_arr = orig_step->v_arr;
	step_copy->v_dep = orig_step->v_dep;
	step_copy->date = orig_step->date;
}

struct ItinStep * create_itin_copy(struct ItinStep *step) {
	struct ItinStep *new_step = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	copy_step_body_vectors_and_date(step, new_step);
	new_step->prev = NULL;
	new_step->num_next_nodes = step->num_next_nodes;
	if(step->num_next_nodes > 0) {
		new_step->next = (struct ItinStep **) malloc(step->num_next_nodes * sizeof(struct ItinStep *));
		for(int i = 0; i < new_step->num_next_nodes; i++) {
			new_step->next[i] = create_itin_copy(step->next[i]);
			new_step->next[i]->prev = new_step;
		}
	} else new_step->next = NULL;
	return new_step;
}

struct ItinStep * create_itin_copy_from_arrival(struct ItinStep *step) {
	struct ItinStep *step_copy = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	copy_step_body_vectors_and_date(step, step_copy);
	step_copy->next = NULL;
	step_copy->num_next_nodes = 0;

	while(step->prev != NULL) {
		step_copy->prev = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		step_copy->prev->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
		step_copy->prev->next[0] = step_copy;
		step_copy->prev->num_next_nodes = 1;

		step = step->prev;
		step_copy = step_copy->prev;
		copy_step_body_vectors_and_date(step, step_copy);
	}


	step_copy->prev = NULL;
	return get_last(step_copy);
}

int is_valid_itinerary(struct ItinStep *step) {
	if(step == NULL || step->body == NULL || step->prev == NULL) return 0;
	while(step != NULL) {
		if(step->body == NULL && step->v_body.x == 0) return 0;
		step = step->prev;
	}

	return 1;
}

void store_step_in_file(struct ItinStep *step, FILE *file, int layer, int variation) {
	fprintf(file, "#%d#%d\n", layer, variation);
	fprintf(file, "Date: %f\n", step->date);
	fprintf(file, "r: %lf, %lf, %lf\n", step->r.x, step->r.y, step->r.z);
	fprintf(file, "v_dep: %f, %f, %f\n", step->v_dep.x, step->v_dep.y, step->v_dep.z);
	fprintf(file, "v_arr: %f, %f, %f\n", step->v_arr.x, step->v_arr.y, step->v_arr.z);
	fprintf(file, "v_body: %f, %f, %f\n", step->v_body.x, step->v_body.y, step->v_body.z);
	fprintf(file, "Next Steps: %d\n", step->num_next_nodes);

	for(int i = 0; i < step->num_next_nodes; i++) {
		store_step_in_file(step->next[i], file, layer + 1, i);
	}
}

void store_itineraries_in_file(struct ItinStep **departures, int num_nodes, int num_deps) {
	char filename[50];  // 14 for date + 4 for .csv + 1 for string terminator
	sprintf(filename, "./Itineraries/test.transfer");
	int num_steps = get_num_of_itin_layers(departures[0]);
	printf("Filesize: ~%.3f MB\n", (double)num_nodes*240/1e6);

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
		store_step_in_file(departures[i], file, 0, i);
	}

	fclose(file);
}

union ItinStepBinHeader {
	struct ItinStepBinHeaderT0 {
		int num_nodes, num_deps, num_layers;
	} t0;

	struct ItinStepBinHeaderT1 {
		int num_nodes, num_deps;
	} t1;
};

union ItinStepBin {
	struct ItinStepBinT0 {
		struct Vector r;
		struct Vector v_dep, v_arr, v_body;
		double date;
		int num_next_nodes;
	} t0;

	struct ItinStepBinT1 {
		double date;
		struct Vector r;
		struct Vector v_dep, v_arr, v_body;
		int body_id;
		int num_next_nodes;
	} t1;
};

union ItinStepBin convert_ItinStep_bin(struct ItinStep *step, int file_type) {
	union ItinStepBin bin_step;
	if(file_type == 0) {
		bin_step.t0.r = step->r;
		bin_step.t0.v_dep = step->v_dep;
		bin_step.t0.v_arr = step->v_arr;
		bin_step.t0.v_body = step->v_body;
		bin_step.t0.date = step->date;
		bin_step.t0.num_next_nodes = step->num_next_nodes;
	} else if(file_type == 1) {
		bin_step.t1.r = step->r;
		bin_step.t1.v_dep = step->v_dep;
		bin_step.t1.v_arr = step->v_arr;
		bin_step.t1.v_body = step->v_body;
		bin_step.t1.date = step->date;
		bin_step.t1.body_id = step->body->id;
		bin_step.t1.num_next_nodes = step->num_next_nodes;
	}

	return bin_step;
}

void convert_bin_ItinStep(union ItinStepBin bin_step, struct ItinStep *step, struct Body *body, int file_type) {
	if(file_type == 0) {
		step->body = body;
		step->r = bin_step.t0.r;
		step->v_arr = bin_step.t0.v_arr;
		step->v_body = bin_step.t0.v_body;
		step->v_dep = bin_step.t0.v_dep;
		step->date = bin_step.t0.date;
		step->num_next_nodes = bin_step.t0.num_next_nodes;
	} else if(file_type == 1) {
		step->body = get_body_from_id(bin_step.t1.body_id);
		step->r = bin_step.t1.r;
		step->v_arr = bin_step.t1.v_arr;
		step->v_body = bin_step.t1.v_body;
		step->v_dep = bin_step.t1.v_dep;
		step->date = bin_step.t1.date;
		step->num_next_nodes = bin_step.t1.num_next_nodes;
	}
}

void store_step_in_bfile(struct ItinStep *step, FILE *file, int file_type) {
	if(file_type == 0) {
		union ItinStepBin bin_step = convert_ItinStep_bin(step, 0);
		fwrite(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
	} else if(file_type == 1) {
		union ItinStepBin bin_step = convert_ItinStep_bin(step, 1);
		fwrite(&bin_step.t1, sizeof(struct ItinStepBinT1), 1, file);
	}
	for(int i = 0; i < step->num_next_nodes; i++) {
		store_step_in_bfile(step->next[i], file, file_type);
	}
}

void store_itineraries_in_bfile(struct ItinStep **departures, int num_nodes, int num_deps, char *filepath, int file_type) {
	// Check if the string ends with ".itins"
	if (strlen(filepath) >= 6 && strcmp(filepath + strlen(filepath) - 6, ".itins") != 0) {
		// If not, append ".itins" to the string
		strcat(filepath, ".itins");
	}

	union ItinStepBinHeader bin_header;
	FILE *file = fopen(filepath, "wb");
	fwrite(&file_type, sizeof(int), 1, file);

	// TYPE 0 --------------------------------------------
	if(file_type == 0) {
		bin_header.t0.num_nodes = num_nodes;
		bin_header.t0.num_deps = num_deps;
		bin_header.t0.num_layers = get_num_of_itin_layers(departures[0]);

		printf("Filesize: ~%.3f MB\n", (double) num_nodes*sizeof(struct ItinStepBinT0)/1e6);

		printf("Number of stored nodes: %d\n", bin_header.t0.num_nodes);
		printf("Number of Departures: %d, Number of Steps: %d\n", num_deps, bin_header.t0.num_layers);

		fwrite(&bin_header.t0, sizeof(struct ItinStepBinHeaderT0), 1, file);

		struct ItinStep *ptr = departures[0];

		// same algorithm as layer counter (part of header)
		while(ptr != NULL) {
			int body_id = (ptr->body != NULL) ? ptr->body->id : 0;
			fwrite(&body_id, sizeof(int), 1, file);
			if(ptr->next != NULL) ptr = ptr->next[0];
			else break;
		}

		int end_of_bodies_designator = -1;
		fwrite(&end_of_bodies_designator, sizeof(int), 1, file);

		for(int i = 0; i < num_deps; i++) {
			store_step_in_bfile(departures[i], file, file_type);
		}
	// TYPE 1 --------------------------------------------
	} else if(file_type == 1) {
		bin_header.t1.num_nodes = num_nodes;
		bin_header.t1.num_deps = num_deps;

		printf("Filesize: ~%.3f MB\n", (double) num_nodes*sizeof(struct ItinStepBinT1)/1e6);

		printf("Number of stored nodes: %d\n", bin_header.t1.num_nodes);
		printf("Number of Departures: %d\n", num_deps);

		fwrite(&bin_header.t1, sizeof(struct ItinStepBinHeaderT1), 1, file);

		int end_of_bodies_designator = -1;
		fwrite(&end_of_bodies_designator, sizeof(int), 1, file);

		for(int i = 0; i < num_deps; i++) {
			store_step_in_bfile(departures[i], file, file_type);
		}
	}
	// ---------------------------------------------------

	fclose(file);
}

void load_step_from_bfile(struct ItinStep *step, FILE *file, struct Body **body, int file_type) {
	union ItinStepBin bin_step;
	// TYPE 0 --------------------------------------------
	if(file_type == 0) {
		fread(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
		convert_bin_ItinStep(bin_step, step, body[0], 0);
	// TYPE 1 --------------------------------------------
	} else if(file_type == 1) {
		fread(&bin_step.t1, sizeof(struct ItinStepBinT1), 1, file);
		convert_bin_ItinStep(bin_step, step, NULL, 1);
	}
	// ---------------------------------------------------

	if(step->num_next_nodes > 0)
		step->next = (struct ItinStep **) malloc(step->num_next_nodes * sizeof(struct ItinStep *));
	else step->next = NULL;

	for(int i = 0; i < step->num_next_nodes; i++) {
		step->next[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
		step->next[i]->prev = step;
		load_step_from_bfile(step->next[i], file, body == NULL ? NULL : body + 1, file_type);
	}
}

struct ItinStep ** load_itineraries_from_bfile(char *filepath) {
	struct ItinStep **departures = NULL;
	union ItinStepBinHeader bin_header;

	FILE *file;
	file = fopen(filepath,"rb");

	int type;
	fread(&type, sizeof(int), 1, file);

	// TYPE 0 --------------------------------------------
	if(type == 0 || type > 3) {
		if(type > 3) {fclose(file); file = fopen(filepath,"rb");}	// no type specified in header
		fread(&bin_header.t0, sizeof(struct ItinStepBinHeaderT0), 1, file);

		int *bodies_id = (int *) malloc(bin_header.t0.num_layers * sizeof(int));
		fread(bodies_id, sizeof(int), bin_header.t0.num_layers, file);

		int buf;
		fread(&buf, sizeof(int), 1, file);

		if(buf != -1) {
			printf("Problems reading itinerary file (Body list or header wrong)\n");
			fclose(file);
			return NULL;
		}

		departures = (struct ItinStep **) malloc(bin_header.t0.num_deps * sizeof(struct ItinStep *));

		struct Body **bodies = (struct Body **) malloc(bin_header.t0.num_layers * sizeof(struct Body *));
		for(int i = 0; i < bin_header.t0.num_layers; i++)
			bodies[i] = (bodies_id[i] > 0) ? get_body_from_id(bodies_id[i]) : NULL;
		free(bodies_id);

		for(int i = 0; i < bin_header.t0.num_deps; i++) {
			departures[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			departures[i]->prev = NULL;
			load_step_from_bfile(departures[i], file, bodies, 0);
		}
		free(bodies);
	// TYPE 1 --------------------------------------------
	} else if(type == 1) {
		fread(&bin_header.t1, sizeof(struct ItinStepBinHeaderT1), 1, file);

		int buf;
		fread(&buf, sizeof(int), 1, file);

		if(buf != -1) {
			printf("Problems reading itinerary file (Body list or header wrong)\n");
			fclose(file);
			return NULL;
		}

		departures = (struct ItinStep **) malloc(bin_header.t1.num_deps * sizeof(struct ItinStep *));

		for(int i = 0; i < bin_header.t1.num_deps; i++) {
			departures[i] = (struct ItinStep *) malloc(sizeof(struct ItinStep));
			departures[i]->prev = NULL;
			load_step_from_bfile(departures[i], file, NULL, 1);
		}
	}
	// ---------------------------------------------------

	fclose(file);
	return departures;
}

int get_num_of_deps_of_itinerary_from_bfile(char *filepath) {
	FILE *file;
	file = fopen(filepath,"rb");
	union ItinStepBinHeader bin_header;
	int num_deps = 0;

	int type;
	fread(&type, sizeof(int), 1, file);

	// TYPE 0 --------------------------------------------
	if(type == 0 || type > 3) {
		if(type > 3) {fclose(file); file = fopen(filepath,"rb");}	// no type specified in header
		fread(&bin_header.t0, sizeof(struct ItinStepBinHeaderT0), 1, file);
		num_deps = bin_header.t0.num_deps;
	// TYPE 1 --------------------------------------------
	} else if(type == 1) {
		fread(&bin_header.t1, sizeof(struct ItinStepBinHeaderT1), 1, file);
		num_deps = bin_header.t1.num_deps;
	}
	// ---------------------------------------------------

	fclose(file);
	return num_deps;
}

void store_single_itinerary_in_bfile(struct ItinStep *itin, char *filepath) {
	if(itin == NULL) return;

	int num_nodes = get_num_of_itin_layers(itin);

	// Check if the string ends with ".itin"
	if (strlen(filepath) >= 5 && strcmp(filepath + strlen(filepath) - 5, ".itin") != 0) {
		// If not, append ".itin" to the string
		strcat(filepath, ".itin");
	}


	printf("Storing Itinerary: %s\n", filepath);

	FILE *file;
	file = fopen(filepath,"wb");

	fwrite(&num_nodes, sizeof(int), 1, file);

	struct ItinStep *ptr = itin;

	// same algorithm as layer counter (part of header)
	while(ptr != NULL) {
		int body_id = (ptr->body != NULL) ? ptr->body->id : 0;
		fwrite(&body_id, sizeof(int), 1, file);
		if(ptr->next != NULL) ptr = ptr->next[0];
		else break;
	}

	int end_of_bodies_designator = -1;
	fwrite(&end_of_bodies_designator, sizeof(int), 1, file);

	ptr = itin;
	while(ptr != NULL) {
		union ItinStepBin bin_step = convert_ItinStep_bin(ptr, 0);
		fwrite(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
		if(ptr->next != NULL) ptr = ptr->next[0];
		else break;
	}

	fclose(file);
}

struct ItinStep * load_single_itinerary_from_bfile(char *filepath) {
	int num_nodes;

	printf("Loading Itinerary: %s\n", filepath);

	FILE *file;
	file = fopen(filepath,"rb");

	fread(&num_nodes, sizeof(int), 1, file);

	int *bodies_id = (int*) malloc(num_nodes * sizeof(int));
	fread(bodies_id, sizeof(int), num_nodes, file);

	int buf;
	fread(&buf, sizeof(int), 1, file);

	if(buf != -1) {
		printf("Problems reading itinerary file (Body list or header wrong)\n");
		fclose(file);
		return NULL;
	}


	struct Body **bodies = (struct Body**) malloc(num_nodes * sizeof(struct Body*));
	for(int i = 0; i < num_nodes; i++) bodies[i] = (bodies_id[i] > 0) ? get_body_from_id(bodies_id[i]) : NULL;
	free(bodies_id);

	struct ItinStep *itin;
	union ItinStepBin bin_step;
	struct ItinStep *last_step = NULL;

	for(int i = 0; i < num_nodes; i++) {
		itin = (struct ItinStep*) malloc(sizeof(struct ItinStep));
		fread(&bin_step.t0, sizeof(struct ItinStepBinT0), 1, file);
		convert_bin_ItinStep(bin_step, itin, bodies[i], 0);
		itin->prev = last_step;
		itin->next = NULL;
		if(last_step != NULL) {
			last_step->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
			last_step->next[0] = itin;
		}
		last_step = itin;
	}

	fclose(file);
	free(bodies);
	return itin;
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

void free_itinerary(struct ItinStep *step) {
	if(step == NULL) return;
	if(step->next != NULL) {
		for(int i = 0; i < step->num_next_nodes; i++) {
			free_itinerary(step->next[i]);
		}
		free(step->next);
	}
	free(step);
}
