#ifndef KSP_DRAWING_H
#define KSP_DRAWING_H

#include <cairo.h>
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "orbit_calculator/itin_tool.h"
#include "transfer_app/porkchop_analyzer_tools.h"
#include "gui/gui_tools/camera.h"


enum CoordAxisLabelType {COORD_LABEL_NUMBER, COORD_LABEL_DATE, COORD_LABEL_DURATION};

void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2);
void draw_celestial_system(Camera *camera, struct System *system, double jd_date);
void draw_body(Camera *camera, struct System *system, struct Body *body, double jd_date);
void draw_orbit(Camera *camera, struct Orbit orbit);
void draw_itinerary(Camera *camera, struct System *system, struct ItinStep *tf, double current_time);
void draw_orbit_2d(cairo_t *cr, struct Vector2D center, double scale, struct Vector r, struct Vector v, struct Body *attractor);
void draw_body_2d(cairo_t *cr, struct Vector2D center, double scale, struct Vector r);
double calc_scale(int area_width, int area_height, struct Body *farthest_body);
void set_cairo_body_color(cairo_t *cr, struct Body *body);
void draw_transfer_point_2d(cairo_t *cr, struct Vector2D center, double scale, struct Vector r);
void draw_trajectory_2d(cairo_t *cr, struct Vector2D center, double scale, struct ItinStep *tf, struct Body *attractor);
void draw_porkchop(cairo_t *cr, double width, double height, struct PorkchopAnalyzerPoint *porkchop, int num_itins, enum LastTransferType last_transfer_type);
void draw_plot(cairo_t *cr, double width, double height, double *x, double *y, int num_points);
void draw_multi_plot(cairo_t *cr, double width, double height, double *x, double **y, int num_plots, int num_points);

#endif //KSP_DRAWING_H
