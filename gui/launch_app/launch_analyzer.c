#include "launch_analyzer.h"
#include "database/lv_database.h"
#include "launch_calculator/lv_profile.h"
#include "tools/analytic_geometry.h"
#include "launch_calculator/launch_sim.h"
#include "launch_calculator/launch_calculator.h"
#include "gui/drawing.h"
#include "gui/gui_manager.h"



enum DispVariable {
	DV_TIME, DV_DWNRNG, DV_ALT, DV_ORBV, DV_SURFV, DV_VERTV, DV_MASS, DV_PITCH, DV_SMA, DV_ECC, DV_INCL
};

#define NUM_DISP_VAR 11

GObject *cb_la_sel_launcher;
GObject *cb_la_sel_profile;
GObject *da_la_disp1;
GObject *da_la_disp2;
GObject *lb_la_res_dur;
GObject *lb_la_res_alt;
GObject *lb_la_res_ap;
GObject *lb_la_res_pe;
GObject *lb_la_res_sma;
GObject *lb_la_res_ecc;
GObject *lb_la_res_incl;
GObject *lb_la_res_orbv;
GObject *lb_la_res_surfv;
GObject *lb_la_res_vertv;
GObject *lb_la_res_spentdv;
GObject *lb_la_res_remdv;
GObject *lb_la_res_remfuel;
GObject *lb_la_res_dwnrng;
GObject *tf_la_sim_incl;
GObject *tf_la_sim_lat;
GObject *tf_la_sim_plmass;
GObject *tf_la_coast_time_after_launch;
GObject *rb_la_dispx[NUM_DISP_VAR];
GObject *rb_la_dispy[NUM_DISP_VAR];

struct LaunchState *launch_state;

int disp1x, disp1y, disp2x, disp2y, curr_sel_disp;

void update_la_display_radios();

struct LaunchDataPoints {
	int num_points;
	double *t;		// time [s]
	double *dwnrng;	// downrange distance [km]
	double *alt; 	// altitude [km]
	double *orbv; 	// orbital speed [m/s]
	double *surfv;	// surface speed [m/s]
	double *vertv;	// vertical speed [m/s]
	double *mass;	// mass [t]
	double *pitch;	// pitch [deg]
	double *sma;	// semi-mayor axis [km]
	double *ecc;	// eccentricity
	double *incl;	// inclination [deg]
	int *stage;
};

struct LaunchDataPoints ldp;

void init_launch_analyzer(GtkBuilder *builder) {
	disp1x = DV_TIME, disp1y = DV_ALT, disp2x = DV_TIME, disp2y = DV_ORBV, curr_sel_disp = 1;
	
	cb_la_sel_profile = gtk_builder_get_object(builder, "cb_la_sel_profile");
	cb_la_sel_launcher = gtk_builder_get_object(builder, "cb_la_sel_launcher");
	da_la_disp1 = gtk_builder_get_object(builder, "da_la_disp1");
	da_la_disp2 = gtk_builder_get_object(builder, "da_la_disp2");
	lb_la_res_dur = gtk_builder_get_object(builder, "lb_la_res_dur");
	lb_la_res_alt = gtk_builder_get_object(builder, "lb_la_res_alt");
	lb_la_res_ap = gtk_builder_get_object(builder, "lb_la_res_ap");
	lb_la_res_pe = gtk_builder_get_object(builder, "lb_la_res_pe");
	lb_la_res_sma = gtk_builder_get_object(builder, "lb_la_res_sma");
	lb_la_res_ecc = gtk_builder_get_object(builder, "lb_la_res_ecc");
	lb_la_res_incl = gtk_builder_get_object(builder, "lb_la_res_incl");
	lb_la_res_orbv = gtk_builder_get_object(builder, "lb_la_res_orbv");
	lb_la_res_surfv = gtk_builder_get_object(builder, "lb_la_res_surfv");
	lb_la_res_vertv = gtk_builder_get_object(builder, "lb_la_res_vertv");
	lb_la_res_spentdv = gtk_builder_get_object(builder, "lb_la_res_spentdv");
	lb_la_res_remdv = gtk_builder_get_object(builder, "lb_la_res_remdv");
	lb_la_res_remfuel = gtk_builder_get_object(builder, "lb_la_res_remfuel");
	lb_la_res_dwnrng = gtk_builder_get_object(builder, "lb_la_res_dwnrng");
	tf_la_sim_incl = gtk_builder_get_object(builder, "tf_la_sim_incl");
	tf_la_sim_lat = gtk_builder_get_object(builder, "tf_la_sim_lat");
	tf_la_sim_plmass = gtk_builder_get_object(builder, "tf_la_sim_plmass");
	tf_la_coast_time_after_launch = gtk_builder_get_object(builder, "tf_la_coast_time_after_launch");

	for(int i = 0; i < NUM_DISP_VAR; i++) {
		char rb_id[30];
		sprintf(rb_id, "rb_la_dispx_%d", i);
		rb_la_dispx[i] = gtk_builder_get_object(builder, rb_id);
		sprintf(rb_id, "rb_la_dispy_%d", i);
		rb_la_dispy[i] = gtk_builder_get_object(builder, rb_id);
	}

	
	update_launcher_dropdown(GTK_COMBO_BOX(cb_la_sel_launcher));
	update_la_display_radios();

	create_combobox_dropdown_text_renderer(cb_la_sel_launcher, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_la_sel_profile, GTK_ALIGN_CENTER);
}

void free_launch_data_points() {
	if(ldp.t != NULL) {
		free(ldp.t);
		free(ldp.dwnrng);
		free(ldp.alt);
		free(ldp.orbv);
		free(ldp.surfv);
		free(ldp.vertv);
		free(ldp.mass);
		free(ldp.pitch);
		free(ldp.sma);
		free(ldp.ecc);
		free(ldp.incl);
		free(ldp.stage);
	}
}

void update_launch_data_points(struct LaunchState *state, struct Body *body) {
	if(state == NULL) return;
	free_launch_data_points();

	double step = 1; // [s]
	double next_point_t = 0;

	ldp.num_points = (int) (get_last_state(state)->t / step) + 2;

	ldp.t = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.dwnrng = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.alt = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.orbv = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.surfv = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.vertv = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.mass = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.pitch = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.sma = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.ecc = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.incl = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.stage = (int*) malloc(sizeof(int) * ldp.num_points);

	state = get_first_state(state);
	int counter = 0;
	struct Orbit orbit;

	double launch_latitude = angle_vec_vec(vec(1,0,0), state->r);

	while(state != NULL) {
		if(state->t >= next_point_t || state->next == NULL) {
			ldp.t[counter] = state->t;
			ldp.dwnrng[counter] = calc_downrange_distance(state->r, state->t, launch_latitude, EARTH())/1000;
			ldp.alt[counter] = (vector_mag(state->r) - body->radius) / 1000;
			ldp.orbv[counter] = vector_mag(state->v);
			ldp.surfv[counter] = vector_mag(calc_surface_velocity_from_osv(state->r, state->v, body));
			ldp.vertv[counter] = calc_vertical_speed_from_osv(state->r, state->v);
			ldp.mass[counter] = state->m / 1000;
			ldp.pitch[counter] = rad2deg(state->pitch);
			orbit = constr_orbit_from_osv(state->r, state->v, EARTH());
			ldp.sma[counter] = orbit.a/1000;
			ldp.ecc[counter] = orbit.e;
			ldp.incl[counter] = rad2deg(orbit.inclination);
			ldp.stage[counter] = state->stage_id;
			counter++;
			next_point_t += step;
		}
		state = state->next;
	}
}

G_MODULE_EXPORT void on_launch_analyzer_disp_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;
	int disp_id = (int) strtol(gtk_widget_get_name(widget), NULL, 10);	// char to int

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0.15,0.15, 0.15);
	cairo_fill(cr);

	double *x, *y;
	switch(disp_id == 1 ? disp1x : disp2x) {
		case DV_TIME: 	x = ldp.t; 		break;
		case DV_DWNRNG: x = ldp.dwnrng;	break;
		case DV_ALT: 	x = ldp.alt;	break;
		case DV_ORBV: 	x = ldp.orbv;	break;
		case DV_SURFV: 	x = ldp.surfv;	break;
		case DV_VERTV: 	x = ldp.vertv;	break;
		case DV_MASS: 	x = ldp.mass;	break;
		case DV_PITCH: 	x = ldp.pitch;	break;
		case DV_SMA: 	x = ldp.sma;	break;
		case DV_ECC: 	x = ldp.ecc;	break;
		case DV_INCL: 	x = ldp.incl;	break;
		default: return;
	}
	switch(disp_id == 1 ? disp1y : disp2y) {
		case DV_TIME: 	y = ldp.t; 		break;
		case DV_DWNRNG: y = ldp.dwnrng;	break;
		case DV_ALT: 	y = ldp.alt;	break;
		case DV_ORBV: 	y = ldp.orbv;	break;
		case DV_SURFV: 	y = ldp.surfv;	break;
		case DV_VERTV: 	y = ldp.vertv;	break;
		case DV_MASS: 	y = ldp.mass;	break;
		case DV_PITCH: 	y = ldp.pitch;	break;
		case DV_SMA: 	y = ldp.sma;	break;
		case DV_ECC: 	y = ldp.ecc;	break;
		case DV_INCL: 	y = ldp.incl;	break;
		default: return;
	}
	if(launch_state != NULL) draw_plot(cr, area_width, area_height, x, y, ldp.num_points);
}

G_MODULE_EXPORT void on_launch_analyzer_disp2_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0.15,0.15, 0.15);
	cairo_fill(cr);

	if(launch_state != NULL) draw_plot(cr, area_width, area_height, ldp.t, ldp.alt, ldp.num_points);
}

void update_launch_result_values(double dur, double alt, double ap, double pe, double sma, double ecc, double incl,
								 double orbv, double surfv, double vertv, double spentdv, double remdv, double remfuel, double dwnrng) {
	char value_string[20];
	sprintf(value_string, "%.0f s", dur);
	gtk_label_set_label(GTK_LABEL(lb_la_res_dur), value_string);
	sprintf(value_string, "%.0f km", alt/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_alt), value_string);
	sprintf(value_string, "%.0f km", ap/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_ap), value_string);
	sprintf(value_string, "%.0f km", pe/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_pe), value_string);
	sprintf(value_string, "%.0f km", sma/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_sma), value_string);
	sprintf(value_string, "%.3f", ecc);
	gtk_label_set_label(GTK_LABEL(lb_la_res_ecc), value_string);
	sprintf(value_string, "%.2f°", rad2deg(incl));
	gtk_label_set_label(GTK_LABEL(lb_la_res_incl), value_string);

	sprintf(value_string, "%.0f m/s", orbv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_orbv), value_string);
	sprintf(value_string, "%.0f m/s", surfv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_surfv), value_string);
	sprintf(value_string, "%.0f m/s", vertv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_vertv), value_string);
	sprintf(value_string, "%.0f m/s", spentdv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_spentdv), value_string);
	sprintf(value_string, "%.0f m/s", remdv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_remdv), value_string);
	sprintf(value_string, "%.0f kg", remfuel);
	gtk_label_set_label(GTK_LABEL(lb_la_res_remfuel), value_string);
	sprintf(value_string, "%.0f km", dwnrng/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_dwnrng), value_string);
}

G_MODULE_EXPORT void on_la_change_launcher(GtkWidget* widget, gpointer data) {
	update_profile_dropdown(GTK_COMBO_BOX(cb_la_sel_launcher), GTK_COMBO_BOX(cb_la_sel_profile));
}


G_MODULE_EXPORT void on_run_launch_simulation(GtkWidget* widget, gpointer data) {
	int launcher_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_la_sel_launcher));
	int profile_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_la_sel_profile));
	struct LV *all_launcher = get_all_launcher();
	int *launcher_ids = get_launcher_ids();

	if(launcher_id < 0 || profile_id < 0) return;

	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(launcher_ids[launcher_id]);

	char *string;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_la_sim_lat));
	double launch_latitude = deg2rad(strtod(string, NULL));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_la_sim_incl));
	double target_incl = deg2rad(strtod(string, NULL));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_la_sim_plmass));
	double payload_mass = strtod(string, NULL) * 1000;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_la_coast_time_after_launch));
	double coast_time_after_launch = strtod(string, NULL);
	
	all_launcher[launcher_id].lp_id = profiles.profile[profile_id].profiletype;
	for(int i = 0; i < 5; i++) all_launcher[launcher_id].lp_params[i] = profiles.profile[profile_id].lp_params[i];
	
	print_LV(&all_launcher[launcher_id]);

	struct Launch_Results lr = run_launch_simulation(all_launcher[launcher_id], payload_mass, launch_latitude, target_incl, 0.001, 1, 1, coast_time_after_launch);

	if(launch_state != NULL) free_launch_states(launch_state);
	launch_state = get_last_state(lr.state);

	struct Body *body = EARTH();

	struct Orbit orbit = constr_orbit_from_osv(launch_state->r, launch_state->v, body);

	update_launch_result_values(
			launch_state->t,
			vector_mag(launch_state->r) - body->radius,
			calc_orbit_apoapsis(orbit),
			calc_orbit_periapsis(orbit),
			orbit.a,
			orbit.e,
			orbit.inclination,
			vector_mag(launch_state->v),
			vector_mag(calc_surface_velocity_from_osv(launch_state->r, launch_state->v, body)),
			calc_vertical_speed_from_osv(launch_state->r, launch_state->v),
			lr.dv,
			lr.rem_dv,
			lr.rf,
			calc_downrange_distance(launch_state->r, launch_state->t, launch_latitude, body)
			);

	update_launch_data_points(launch_state, body);

//	for(int i = 0; i < ldp.num_points; i++) {
//		printf("%f s   %f km   %f m/s   %f m/s     %f m/s     %f t      %f°    %f°  %f   %d\n", ldp.t[i], ldp.alt[i], ldp.surfv[i], ldp.orbv[i], ldp.vertv[i], ldp.mass[i],
//			   ldp.pitch[i], ldp.incl[i], ldp.ecc[i], ldp.stage[i]);
//	}
}

void update_la_display_radios() {
	int dispx = curr_sel_disp == 1 ? disp1x : disp2x;
	int dispy = curr_sel_disp == 1 ? disp1y : disp2y;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_la_dispx[dispx]), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_la_dispy[dispy]), TRUE);
}

G_MODULE_EXPORT void on_la_disp_sel(GtkWidget* widget, gpointer data) {
	curr_sel_disp = (int) strtol(gtk_widget_get_name(widget), NULL, 10);	// char to int
	update_la_display_radios();
}

G_MODULE_EXPORT void on_change_la_display_xvariable(GtkWidget* widget, gpointer data) {
	int id = (int) strtol(gtk_widget_get_name(widget), NULL, 10);	// char to int
	if(curr_sel_disp == 1) {
		disp1x = id;
		gtk_widget_queue_draw(GTK_WIDGET(da_la_disp1));
	} else if(curr_sel_disp == 2) {
		disp2x = id;
		gtk_widget_queue_draw(GTK_WIDGET(da_la_disp2));
	}
}

G_MODULE_EXPORT void on_change_la_display_yvariable(GtkWidget* widget, gpointer data) {
	int id = (int) strtol(gtk_widget_get_name(widget), NULL, 10);	// char to int
	if(curr_sel_disp == 1) {
		disp1y = id;
		gtk_widget_queue_draw(GTK_WIDGET(da_la_disp1));
	} else if(curr_sel_disp == 2) {
		disp2y = id;
		gtk_widget_queue_draw(GTK_WIDGET(da_la_disp2));
	}
}

void close_launch_analyzer() {
	if(launch_state != NULL) free_launch_states(launch_state);
	launch_state = NULL;
	free_launch_data_points();
}
