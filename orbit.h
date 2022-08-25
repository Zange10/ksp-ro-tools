#ifndef ORBIT
#define ORBIT

#include "celestial_bodies.h"


struct Vector {
    double x;
    double y;
};

double vector_magnitude(struct Vector v);

double cross_product(struct Vector v1, struct Vector v2);



struct Orbit {
    struct Body *body;  // parent body
    double apoapsis;    // highest point in orbit
    double periapsis;   // lowest point in orbit
    double a;           // semi-major axis
    double inclination; // inclination
    double e;           // eccentricity
    double period;      // orbital period
    double theta;       // true anomaly / phase angle of orbit (starting point periapsis)
};

struct Orbit init_orbit(struct Body *body);

void update_orbit_param(struct Orbit *orbit, double r, double vv, double vh);

void calc_semi_major_axis(struct Orbit *orbit, double r, double v);

void calc_eccentricity(struct Orbit *orbit, double r_mag, struct Vector v);

void calc_apsides(struct Orbit *orbit);

void calc_true_anomaly(struct Orbit *orbit, double r, double vv);

double calc_horizontal_speed(struct Orbit *orbit, double r_mag, double v_mag);

#endif