#ifndef LAUNCH_CALCULATOR
#define LAUNCH_CALCULATOR

// initialize vessel (Thrust: F[kN], initial mass: m0[t], burn rate: br[kg/s])
struct  Vessel init_vessel(double F, double m0, double br);
// initialize flight
struct  Flight init_flight(struct Body *body);
// initialize body
struct  Body init_body();
// Prints parameters specific to the vessel
void    print_vessel_info(struct Vessel *v);
// Prints parameters specific to the flight
void    print_flight_info(struct Flight *f);

// calculate parameters after launch
void calculate_launch();

// Calculates vessel and flight parameters over time
void    calculate_flight(struct Vessel *v, struct Flight *f, double t);
// update parameters of vessel for the point in time t of the flight
void    update_vessel(struct Vessel *v, double t);
// update parameters of the flight for the point in time t of the flight
void    update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double t);

// get vessel's pitch after time t
double  get_pitch(double t);
// get vessel's mass after time t
double  get_ship_mass(struct Vessel *v, double t);
// get vessel's acceleration (Thrust) after time  t
double  get_ship_acceleration(struct Vessel *v, double t);
// get vessel's horizontal acceleration (Thrust) after time t
double  get_ship_hacceleration(struct Vessel *v, double t);
// get vessels's vertical acceleration (Thrust) after time t
double  get_ship_vacceleration(struct Vessel *v, double t);

// calculate centrifugal acceleration due to the vessel's horizontal velocity
double  calc_centrifugal_acceleration(struct Flight *f);
// calculate gravitational acceleration at a given distance to the center of the body
double  calc_grav_acceleration(struct Flight *f);
// calculate acceleration towards body without the vessel's thrust (g-ac)
double  calc_balanced_acceleration(struct Flight *f);
// calculate acceleration towards body with vessel's thrust
double  calc_vertical_acceleration(struct Vessel *v, struct Flight *f);
// calculate overall velocity with given vertical and horizontal velocity (pythagoras)
double  calc_velocity(double vh, double vv);

// calculate current Apoapsis of flight/orbit
double  calc_Apoapsis(struct Flight *f);

// integration for a given interval (numerical integration, midpoint/rectangle rule)
// I = ( (f(a)+f(b))/2 ) * step
double  integrate(double fa, double fb, double step);
// transforms degrees to radians
double  deg_to_rad(double deg);

#endif