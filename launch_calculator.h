#ifndef LAUNCH_CALCULATOR
#define LAUNCH_CALCULATOR

// return new instance of struct vessel with selected parameters set to 0
struct  Vessel init_vessel();
// modify vessel with parameters of next stage (F_sl [N], F_vac [N], m0 [kg], me [kg], br [kg/s])
void    init_vessel_next_stage(struct Vessel *vessel, double F_sl, double F_vac, double m0, double br);
// initialize flight
struct  Flight init_flight(struct Body *body, double latitude);
// Prints parameters specific to the vessel
void    print_vessel_info(struct Vessel *v);
// Prints parameters specific to the flight
void    print_flight_info(struct Flight *f);

// start launch calculations
void    launch_calculator();
// calculate parameters during launch 
void    calculate_launch(struct LV lv);

// calculate expended Delta-V after t with initial mass m0, static thrust F and burn rate (integral of a=F/m(t))
double  calculate_dV(double F, double m0, double t, double burn_rate);

// Calculates vessel and flight parameters over time (end is defined by T)
double* calculate_stage_flight(struct Vessel *v, struct Flight *f, double T, int number_of_stages, double *flight_data);
// set starting parameters for flight
void    start_stage(struct Vessel *v, struct Flight *f);
// update parameters of vessel for the point in time t of the flight
void    update_vessel(struct Vessel *v, double t, double p, double h);
// update parameters of the flight for the point in time t of the flight
void    update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double t, double step);
// update position and horizontal and vertical speed
void    calc_pos_in_orbit(struct Flight *f);

// get atmospheric pressure p at height h with the scale height of the parent body
double  get_atmo_press(double h, double scale_height);
// calculate acceleration due to aerodynamic drag with given velocity and atmospheric pressure
double  calc_aerodynamic_drag(double p, double v);
// get thrust at current atmosperic pressure
double  get_thrust(double F_vac, double F_sl, double p);
// get vessel's pitch after time t
double  get_pitch(double t);

// calculate centrifugal acceleration due to the vessel's horizontal speed
double  calc_centrifugal_acceleration(struct Flight *f);
// calculate gravitational acceleration at a given distance to the center of the body
double  calc_grav_acceleration(struct Flight *f);
// calculate acceleration towards body with vessel's thrust, pitch and drag
double  calc_vertical_acceleration(double vertical_a_thrust, double balanced_a, double drag_a, double vert_speed, double v);
// calculate acceleration towards body without the vessel's thrust (g-ac)
double  calc_balanced_acceleration(double g, double centri_a);
// calculate horizontal acceleration with vessel's thrust, pitch and drag
double  calc_horizontal_acceleration(double horizontal_a_thrust, double drag_a,  double hor_speed, double v);
// calculate overall velocity with given vertical and horizontal speed (pythagoras)
double  calc_velocity(double vh, double vv);

// calculate horizontal speed in new frame of reference
double  calc_change_of_reference_frame(struct Flight *f, struct Flight *last_f, double step);
// calculate current Apoapsis of flight/orbit
double  calc_Apoapsis(struct Flight f);

// integration for a given interval (numerical integration, midpoint/rectangle rule)
// I = ( (f(a)+f(b))/2 ) * step
double  integrate(double fa, double fb, double step);
// transforms degrees to radians
double  deg_to_rad(double deg);

// store current flight parameters in addition to the already stored flight parameters
void    store_flight_data(struct Vessel *v, struct Flight *f, double **data);

#endif