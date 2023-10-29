#include "analytic_geometry.h"
#include <math.h>
#include <stdio.h>


struct Vector vec(double x, double y, double z) {
    struct Vector v = {x, y, z};
    return v;
}

struct Vector add_vectors(struct Vector v1, struct Vector v2) {
    struct Vector v;
    v.x = v1.x+v2.x;
    v.y = v1.y+v2.y;
    v.z = v1.z+v2.z;
    return v;
}

double vector_mag(struct Vector v) {
    return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

double vector2d_mag(struct Vector2D v) {
    return sqrt(v.x*v.x + v.y*v.y);
}

struct Vector norm_vector(struct Vector v) {
    double mag = vector_mag(v);
    return scalar_multiply(v, 1 / mag);
}

struct Vector2D norm_vector2d(struct Vector2D v) {
    double mag = vector2d_mag(v);
    return scalar_multipl2d(v, 1 / mag);
}

struct Vector2D rotate_vector2d(struct Vector2D n, double gamma) {
    struct Vector2D v_rot;
    gamma *= -1; // clockwise rotation
    v_rot.x = cos(gamma) * n.x - sin(gamma) * n.y;
    v_rot.y = sin(gamma) * n.x + cos(gamma) * n.y;
    return v_rot;
}

struct Vector scalar_multiply(struct Vector v, double scalar) {
    v.x *= scalar;
    v.y *= scalar;
    v.z *= scalar;
    return v;
}

struct Vector2D scalar_multipl2d(struct Vector2D v, double scalar) {
    v.x *= scalar;
    v.y *= scalar;
    return v;
}

double dot_product(struct Vector v1, struct Vector v2) {
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

double dot_product2d(struct Vector2D v1, struct Vector2D v2) {
    return v1.x*v2.x + v1.y*v2.y;
}

struct Vector cross_product(struct Vector v1, struct Vector v2) {
    struct Vector v;
    v.x = v1.y*v2.z - v1.z*v2.y;
    v.y = v1.z*v2.x - v1.x*v2.z;
    v.z = v1.x*v2.y - v1.y*v2.x;
    return v;
}

struct Plane constr_plane(struct Vector loc, struct Vector u, struct Vector v) {
    struct Plane p;
    p.loc = loc;
    p.u = u;
    p.v = v;
    return p;
}

struct Vector calc_plane_norm_vector(struct Plane p) {
    return cross_product(p.v, p.u);
}

struct Vector2D inverse_vector2d(struct Vector2D v) {
    v.x = 1/v.x;
    v.y = 1/v.y;
    return v;
}

double angle_vec_vec(struct Vector v1, struct Vector v2) {
    return fabs(acos(dot_product(v1,v2) / (vector_mag(v1)* vector_mag(v2))));
}

double angle_plane_vec(struct Plane p, struct Vector v) {
    struct Vector n = cross_product(p.u, p.v);
    return M_PI/2 - angle_vec_vec(n, v);
}

double angle_plane_plane(struct Plane p1, struct Plane p2) {
    struct Vector n1 = cross_product(p1.u, p1.v);
    struct Vector n2 = cross_product(p2.u, p2.v);
    return angle_vec_vec(n1, n2);
}

double angle_vec_vec_2d(struct Vector2D v1, struct Vector2D v2) {
    return fabs(acos(dot_product2d(v1,v2) / (vector2d_mag(v1)* vector2d_mag(v2))));
}

struct Vector calc_intersecting_line_dir(struct Plane p1, struct Plane p2) {
    double M[3][5] = {
            {p1.u.x, p1.v.x, -p2.u.x, -p2.v.x, p2.loc.x-p1.loc.x},
            {p1.u.y, p1.v.y, -p2.u.y, -p2.v.y, p2.loc.y-p1.loc.y},
            {p1.u.z, p1.v.z, -p2.u.z, -p2.v.z, p2.loc.z-p1.loc.z}
    };

    // make 0-triangle
    for(int i = 4; i >= 0; i--) M[1][i] = M[0][0]*M[1][i] - M[1][0] * M[0][i];  // I  -> II
    for(int i = 4; i >= 0; i--) M[2][i] = M[0][0]*M[2][i] - M[2][0] * M[0][i];  // I  -> III
    for(int i = 4; i >= 0; i--) M[2][i] = M[1][1]*M[2][i] - M[2][1] * M[1][i];  // II -> III

    double a = M[2][2];
    double b = M[2][3];
    double c = M[2][4];

    // if planes are equal (inside ecliptic for p_0)
    // return p2.u because for earth ecliptic cases draw line from Sun to Earth
    if(a == 0) return scalar_multiply(p2.u, 1);

    /*
    struct Vector loc = {
            p2.loc.x + c/a*p2.u.x,
            p2.loc.y + c/a*p2.u.y,
            p2.loc.z + c/a*p2.u.z
    }; */

    struct Vector dir = {
            -b/a*p2.u.x + p2.v.x,
            -b/a*p2.u.y + p2.v.y,
            -b/a*p2.u.z + p2.v.z,
    };

    return dir;
}



void print_vector(struct Vector v) {
    printf("\n%f, %f, %f (%f)\n", v.x, v.y, v.z, vector_mag(v));
    printf("x: %f\ny: %f\nz: %f\n", v.x, v.y, v.z);
}

void print_vector2d(struct Vector2D v) {
    printf("\nx: %f\ny: %f\n", v.x, v.y);
}

double deg2rad(double deg) {
    return deg/180*M_PI;
}

double rad2deg(double rad) {
    return rad/M_PI*180;
}

double pi_norm(double rad) {
    while(rad > 2*M_PI) rad -= 2*M_PI;
    while(rad < 0) rad += 2*M_PI;
    return rad;
}
