//
// Created by niklas on 08.01.24.
//
#include <stdlib.h>
#include <stdio.h>

#include "drawing.h"
#include "math.h"


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

void draw_transfer(cairo_t *cr, struct Vector2D center, double scale, struct Vector r) {
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

void draw_trajectory(cairo_t *cr, struct Vector2D center, double scale, struct TransferData *tf0, struct TransferData *tf1, struct Ephem **ephems) {
	cairo_set_source_rgb(cr, 0, 1, 0);
	struct OSV osv0 = osv_from_ephem(ephems[tf0->body->id-1], tf0->date, SUN());
	struct OSV osv1 = osv_from_ephem(ephems[tf1->body->id-1], tf1->date, SUN());
	double dt = (tf1->date-tf0->date)*24*60*60;
	double data[3];
	struct Transfer transfer = calc_transfer(circcap, tf0->body, tf1->body, osv0.r, osv0.v, osv1.r, osv1.v, dt, data);
	tf0->dv = tf0->prev == NULL ? data[1] : 0;
	tf1->dv = tf1->next == NULL ? data[2] : 0;
	
	if(tf0->prev != NULL) {
		double t[3] = {tf0->prev->date, tf0->date, tf1->date};
		struct OSV osv_prev = osv_from_ephem(ephems[tf0->prev->body->id-1], tf0->prev->date, SUN());
		struct OSV osvs[3] = {osv_prev, osv0, osv1};
		struct Body *bodies[3] = {tf0->prev->body, tf0->body, tf1->body};
		if(!is_flyby_viable(t, osvs, bodies)) cairo_set_source_rgb(cr, 1, 0, 0);
	}
	
	int steps = 1000;
	struct Vector r = transfer.r0;
	struct Vector v = transfer.v0;
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