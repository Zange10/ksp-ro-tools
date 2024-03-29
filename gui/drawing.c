#include <stdio.h>

#include "drawing.h"
#include "math.h"
#include <stdlib.h>


void draw_orbit(cairo_t *cr, struct Vector2D center, double scale, struct Vector r, struct Vector v, struct Body *attractor) {
	int steps = 100;
	struct OSV last_osv = {r,v};
	for(int i = 1; i <= steps; i++) {
		double theta = 2*M_PI/steps * i;
		struct OSV osv = propagate_orbit_theta(r,v,theta,attractor);
		// y negative, because in GUI y gets bigger downwards
		struct Vector2D p1 = {last_osv.r.x, -last_osv.r.y};
		struct Vector2D p2 = {osv.r.x, -osv.r.y};
		
		p1 = scalar_multipl2d(p1,scale);
		p2 = scalar_multipl2d(p2,scale);
		
		draw_stroke(cr, add_vectors2d(p1,center), add_vectors2d(p2,center));
		last_osv = osv;
	}
}

void draw_body(cairo_t *cr, struct Vector2D center, double scale, struct Vector r) {
	int body_radius = 5;
	r = scalar_multiply(r, scale);
	// y negative, because in GUI y gets bigger downwards
	r.y *= -1;
	cairo_arc(cr, center.x+r.x, center.y+r.y,body_radius,0,M_PI*2);
	cairo_fill(cr);
}

void draw_transfer_point(cairo_t *cr, struct Vector2D center, double scale, struct Vector r) {
	int cross_length = 4;
	cairo_set_source_rgb(cr, 1, 0, 0);
	r = scalar_multiply(r, scale);
	// y negative, because in GUI y gets bigger downwards
	r.y *= -1;
	for(int i = -1; i<=1; i+=2) {
		struct Vector2D p1 = {r.x - cross_length, r.y + cross_length*i};
		struct Vector2D p2 = {r.x + cross_length, r.y - cross_length*i};
		draw_stroke(cr, add_vectors2d(center, p1), add_vectors2d(center, p2));
	}
}




// Rework trajectory drawing -> OSV + dt   ----------------------------------------------
void draw_trajectory(cairo_t *cr, struct Vector2D center, double scale, struct ItinStep *tf) {
	// if double swing-by is not worth drawing
	if(tf->body == NULL && tf->v_body.x == 0) return;

	cairo_set_source_rgb(cr, 0, 1, 0);
	struct ItinStep *prev = tf->prev;
	// skip not working double swing-by
	if(tf->prev->body == NULL && tf->prev->v_body.x == 0) prev = tf->prev->prev;
	double dt = (tf->date-prev->date)*24*60*60;

	if(prev->prev != NULL && prev->body != NULL) {
		double t[3] = {prev->prev->date, prev->date, tf->date};
		struct OSV osv0 = {prev->prev->r, prev->prev->v_body};
		struct OSV osv1 = {prev->r, prev->v_body};
		struct OSV osv2 = {tf->r, tf->v_body};
		struct OSV osvs[3] = {osv0, osv1, osv2};
		struct Body *bodies[3] = {prev->prev->body, prev->body, tf->body};
		if(!is_flyby_viable(t, osvs, bodies)) cairo_set_source_rgb(cr, 1, 0, 0);
	}

	int steps = 1000;
	struct Vector r = prev->r;
	struct Vector v = tf->v_dep;
	struct OSV last_osv = {r,v};
	for(int i = 1; i <= steps; i++) {
		double time = dt/steps * i;
		struct OSV osv = propagate_orbit_time(r,v,time,SUN());
		// y negative, because in GUI y gets bigger downwards
		struct Vector2D p1 = {last_osv.r.x, -last_osv.r.y};
		struct Vector2D p2 = {osv.r.x, -osv.r.y};
		
		p1 = scalar_multipl2d(p1,scale);
		p2 = scalar_multipl2d(p2,scale);
		
		draw_stroke(cr, add_vectors2d(p1,center), add_vectors2d(p2,center));
		last_osv = osv;
	}
}


void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2) {
	cairo_set_line_width(cr, 1);
	cairo_move_to(cr, p1.x, p1.y);
	cairo_line_to(cr, p2.x, p2.y);
	cairo_stroke(cr);
}

double calc_scale(int area_width, int area_height, int highest_id) {
	if(highest_id == 0) return 1e-9;
	struct Body *body = get_body_from_id(highest_id);
	double apoapsis = body->orbit.apoapsis;
	int wh = area_width < area_height ? area_width : area_height;
	return 1/apoapsis*wh/2.2;	// divided by 2.2 because apoapsis is only one side and buffer
}

void set_cairo_body_color(cairo_t *cr, int id) {
	switch(id) {
		case 0: cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); break;	// Sun
		case 1: cairo_set_source_rgb(cr, 0.3, 0.3, 0.3); break;	// Mercury
		case 2: cairo_set_source_rgb(cr, 0.6, 0.6, 0.2); break;	// Venus
		case 3: cairo_set_source_rgb(cr, 0.2, 0.2, 1.0); break;	// Earth
		case 4: cairo_set_source_rgb(cr, 1.0, 0.2, 0.0); break;	// Mars
		case 5: cairo_set_source_rgb(cr, 0.6, 0.4, 0.2); break;	// Jupiter
		case 6: cairo_set_source_rgb(cr, 0.8, 0.8, 0.6); break;	// Saturn
		case 7: cairo_set_source_rgb(cr, 0.2, 0.6, 1.0); break;	// Uranus
		case 8: cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); break;	// Neptune
		case 9: cairo_set_source_rgb(cr, 0.7, 0.7, 0.7); break;	// Pluto
		default:cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); break;
	}
}

void draw_center_aligned_text(cairo_t *cr, double x, double y, char *text) {
	// Calculate text width
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	double text_width = extents.width;

	// Position and draw text to the left
	cairo_move_to(cr, x - text_width/2, y); // adjust starting position to the left
	cairo_show_text(cr, text);
}

void draw_right_aligned_text(cairo_t *cr, double x, double y, char *text) {
	// Calculate text width
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	double text_width = extents.width;

	// Position and draw text to the left
	cairo_move_to(cr, x - text_width, y); // adjust starting position to the left
	cairo_show_text(cr, text);
}

void draw_right_aligned_int(cairo_t *cr, double x, double y, int number) {
	char text[16];
	sprintf(text, "%d", number);
	draw_right_aligned_text(cr, x, y, text);
}

void draw_data_point(cairo_t *cr, double x, double y, double radius) {
	cairo_arc(cr, x, y, radius,0,M_PI*2);
	cairo_fill(cr);
}

void draw_porkchop(cairo_t *cr, double width, double height, const double *porkchop, int fb0_pow1) {
	// Set text color
	cairo_set_source_rgb(cr, 1, 1, 1);

	// coordinate system
	struct Vector2D origin = {45, height-40};
	draw_stroke(cr, vec2D(origin.x, 0), vec2D(origin.x, origin.y));
	draw_stroke(cr, vec2D(origin.x, origin.y), vec2D(width, origin.y));

	// Set font options
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12.0);
	int half_font_size = 5;


	int num_itins = (int) (porkchop[0]/5);


	double min_dur = porkchop[1 + 1], max_dur = porkchop[1 + 1];
	double min_date = porkchop[0+1], max_date = porkchop[0+1];
	double min_dv = porkchop[2+1]+porkchop[3+1]+porkchop[4+1]*fb0_pow1;
	double max_dv = porkchop[2+1]+porkchop[3+1]+porkchop[4+1]*fb0_pow1;
	int min_dv_ind = 0;
	double dv, date, dur;

	for(int i = 1; i < num_itins; i++) {
		int index = 1+i*5;
		dv = porkchop[index+2]+porkchop[index+3]+porkchop[index+4]*fb0_pow1;
		date = porkchop[index+0];
		dur = porkchop[index+1];

		if(dv < min_dv) {
			min_dv = dv;
			min_dv_ind = i;
		}
		else if(dv > max_dv) max_dv = dv;
		if(date < min_date) min_date = date;
		else if(date > max_date) max_date = date;
		if(dur < min_dur) min_dur = dur;
		else if(dur > max_dur) max_dur = dur;
	}

	double ddate = max_date-min_date;
	double ddur = max_dur - min_dur;

	double margin = 0.05;

	min_date = min_date-ddate*margin;
	max_date = max_date+ddate*margin;
	min_dur = min_dur - ddur * margin;
	max_dur = max_dur + ddur * margin;

	// gradients
	double m_dur, m_date;
	m_date = (width-origin.x)/(max_date - min_date);
	m_dur = -origin.y/(max_dur - min_dur); // negative, because positive is down

	int num_durs = 8;
	int num_dates = 5;
	double min_date_label = floor(min_date+2.0/3*(max_date-min_date)/num_dates);
	double min_dur_label = floor(min_dur+2.0/3*(max_dur-min_dur)/num_durs);
	double x_label_tick = ceil(ddate/num_dates), y_label_tick = ceil(ddur/num_durs);

	double y_label_x = 40,	x_label_y = height - 15;

	// y-labels and y grid
	for(int i = 0; i < num_durs; i++) {
		int label = (int) (min_dur_label + i * y_label_tick);
		double y = (label-min_dur)*m_dur + origin.y;
		cairo_set_source_rgb(cr, 1, 1, 1);
		draw_right_aligned_int(cr, y_label_x, y+half_font_size, label);
		cairo_set_source_rgb(cr, 0, 0, 0);
		draw_stroke(cr, vec2D(origin.x, y), vec2D(width, y));
	}

	// x-labels and x grid
	char date_string[32];
	for(int i = 0; i < num_dates; i++) {
		int label = (int) (min_date_label + i * x_label_tick);
		double x = (label-min_date)*m_date + origin.x;
		date_to_string(convert_JD_date(label), date_string, 0);
		cairo_set_source_rgb(cr, 1, 1, 1);
		draw_center_aligned_text(cr, x, x_label_y, date_string);
		cairo_set_source_rgb(cr, 0, 0, 0);
		draw_stroke(cr, vec2D(x, origin.y), vec2D(x, 0));
	}

	// data
	double color_bias;
	for(int i = num_itins-1; i >= -1; i--) {
		int index = i >= 0 ? 1+i*5 : 1+min_dv_ind*5;

		dv = porkchop[index+2]+porkchop[index+3]+porkchop[index+4]*fb0_pow1;
		date = porkchop[index+0];
		dur = porkchop[index+1];

		// color coding
		color_bias = (dv - min_dv) / (max_dv - min_dv);
		double r = i < 0 ? 1 : color_bias;
		double g = i < 0 ? 0 : 1-color_bias;
		double b = i < 0 ? 0 : 4*pow(color_bias-0.5,2);
		cairo_set_source_rgb(cr, r,g,b);

		struct Vector2D data_point = vec2D(origin.x + m_date*(date - min_date), origin.y + m_dur * (dur - min_dur));
		draw_data_point(cr, data_point.x, data_point.y, i >= 0 ? 2 : 5);
	}
}
