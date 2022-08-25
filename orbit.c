#include <stdio.h>
#include <math.h>
#include "orbit.h"


double vector_magnitude(struct Vector v) {
    return sqrt(v.x*v.x + v.y*v.y);
}

double cross_product(struct Vector v1, struct Vector v2) {
    return v1.x*v2.y - v1.y*v2.x;
}




struct Orbit init_orbit(struct Body *body) {
    struct Orbit orbit;
    orbit.body = body;
    orbit.apoapsis = 0;
    orbit.periapsis = 0;
    orbit.a = 0;
    orbit.inclination = 0;
    orbit.e = 0;
    orbit.period = 0;
    orbit.theta = 0;
    return orbit;
}

void update_orbit_param(struct Orbit *orbit, double r, double vv, double vh) {
    struct Vector v = {vh, vv};
    calc_semi_major_axis(orbit, r, vector_magnitude(v));
    calc_eccentricity(orbit, r, v);
    calc_apsides(orbit);
}

void calc_semi_major_axis(struct Orbit *orbit, double r, double v) {
    double mu = orbit->body->mu;
    orbit->a = (mu*r)/(2*mu-r*v*v);
}

void calc_eccentricity(struct Orbit *orbit, double r_mag, struct Vector v) {
    struct Vector r  = {0,r_mag};         // current position of vessel
    double h = cross_product(r,v);  // angular momentum
    struct Vector e;                // eccentricity vector
    e.x = r.x/vector_magnitude(r) - (h*v.y) / orbit->body->mu;
    e.y = r.y/vector_magnitude(r) + (h*v.x) / orbit->body->mu;
    orbit->e = vector_magnitude(e);
}


void calc_apsides(struct Orbit *orbit) {
    orbit->apoapsis = orbit->a * (1 + orbit->e);
    orbit->periapsis= orbit->a * (1 - orbit->e);
}

void calc_true_anomaly(struct Orbit *orbit, double r, double vv) {
    double temp1 = orbit->a*(1 - pow(orbit->e,2));
    double temp2 = r*orbit->e;
    double temp3 = 1/orbit->e;
    orbit->theta = acos(temp1/temp2-temp3);
    if(vv < 0) orbit->theta = 2*M_PI - orbit->theta;
}

double calc_horizontal_speed(struct Orbit *orbit, double r_mag, double v_mag) {
    struct Vector p;    // point of vessel's location
    struct Vector f2;   // empty focus
    struct Vector f2p;  // vector pointing from f2 to p
    p.x = -r_mag * cos(orbit->theta);
    p.y =  r_mag * sin(orbit->theta);
    f2.x= 2 * orbit->a * orbit->e;
    f2.y= 0;
    f2p.x = p.x - f2.x;
    f2p.y = p.y - f2.y;

    double beta  = M_PI - orbit->theta;
    double gamma = atan(f2p.y/f2p.x);
    if(f2p.x > 0) gamma = M_PI - gamma;
    double alpha = 0.5 * (M_PI - beta - gamma);
    double phi   = 0.5*M_PI - beta - alpha;

    struct Vector v;    // velocity vector of vessel
    v.x =  v_mag*cos(phi);
    v.y = -v_mag*sin(phi);

    return abs( (v.y*p.x - v.x*p.y) / r_mag );
}