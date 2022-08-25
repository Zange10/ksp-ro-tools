#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "launch_calculator.h"
#include "csv_writer.h"
#include "tool_funcs.h"
#include "lv_profile.h"
#include "celestial_bodies.h"
#include "orbit.h"

struct Vessel {
    double F_vac;       // Thrust produced by the engines in a vacuum [N]
    double F_sl;        // Thrust produced by the engines at sea level [N]
    double F;           // current Thrust produced by the engines [N]
    double mass;        // vessel mass [kg]
    double m0;          // initial mass (mass at t0) [kg]
    double burn_rate;   // burn rate of all running engines combined [kg/s]
    double pitch;       // pitch of vessel during flight [°]
    double a;           // acceleration due to Thrust [m/s²]
    double ah;          // horizontal acceleration due to Thrust and pitch [m/s²]
    double av;          // vertical acceleration due to Thrust and pitch [m/s²]
    double dV;          // spent delta-V [m/s]
};

struct Flight {
    struct Body *body;
    struct Orbit orbit;
    double t;       // time passed since t0 [s]
    double p;       // atmospheric pressure [Pa]
    double D;       // atmospheric Drag [N]
    double ad;      // acceleration due to aerodynamic drag [m/s²]
    double ah;      // current horizontal acceleration due to thrust and with drag [m/s²]
    double g;       // gravitational acceleration [m/s²]
    double ac;      // negative centripetal force due to horizontal speed [m/s²]
    double ab;      // gravitational a subtracted by cetrifucal a [m/s²]
    double av;      // current vertical acceleration due to Thrust, pitch, gravity and velocity [m/s²]
    double vh_s;    // horizontal surface speed [m/s]
    double vh;      // horizontal orbital speed [m/s]
    double vv;      // vertical speed [m/s]
    double v_s;     // overall surface velocity [m/s]
    double v;       // overall orbital velocity [m/s]
    double h;       // altitude above sea level [m]
    double r;       // distance to center of body [m]
    double Ap;      // highest point of orbit in reference to h [m]
    double s;       // distance travelled downrange [m]
};


struct Vessel init_vessel() {
    struct Vessel new_vessel;
    new_vessel.mass = 0;
    new_vessel.pitch = 0;
    new_vessel.a = 0;
    new_vessel.ah = 0;
    new_vessel.av = 0;
    new_vessel.dV = 0;
    return new_vessel;
}

void init_vessel_next_stage(struct Vessel *vessel, double F_sl, double F_vac, double m0, double br) {
    vessel -> F_vac = F_vac;
    vessel -> F_sl = F_sl;
    vessel -> m0 = m0;
    vessel -> burn_rate = br;
}

struct Flight init_flight(struct Body *body, double latitude) {
    struct Flight new_flight;
    new_flight.body = body;
    new_flight.orbit = init_orbit(body);
    new_flight.t = 0;
    new_flight.vh_s = 0;
    // horizontal surface speed of body at equator -> circumference devided by rotational period [m/s]
    double vh_at_equator = (2*body->radius*M_PI) / body->rotation_period;
    new_flight.vh = vh_at_equator*cos(deg_to_rad(latitude));
    new_flight.vv = 0;
    new_flight.v_s = 0;
    new_flight.v = 0;
    new_flight.h = 0;
    new_flight.r = new_flight.h + new_flight.body->radius;
    new_flight.s = 0;
    return new_flight;
}


void print_vessel_info(struct Vessel *v) {
    printf("\n______________________\nVESSEL:\n\n");
    printf("Thrust:\t\t%g kN\n", v -> F/1000);
    printf("Mass:\t\t%g kg\n", v -> mass);
    printf("Initial mass:\t%g kg\n", v -> m0);
    printf("Burn rate:\t%g kg/s\n", v -> burn_rate);
    printf("Pitch:\t\t%g°\n", v -> pitch);
    printf("Acceleration:\t%g m/s²\n", v -> a);
    printf("Horizontal a:\t%g m/s²\n", v -> ah);
    printf("Vertical a:\t%g m/s²\n", v -> av);
    printf("Used Delta-V:\t%g m/s\n", v -> dV);
    printf("______________________\n\n");
}

void print_flight_info(struct Flight *f) {
    printf("\n______________________\nFLIGHT:\n\n");
    printf("Time:\t\t\t%.2f s\n", f -> t);
    printf("Altitude:\t\t%g km\n", f -> h/1000);
    printf("Vertical v:\t\t%g m/s\n", f -> vv);
    printf("Horizontal surfV:\t%g m/s\n", f -> vh_s);
    printf("surfVelocity:\t\t%g m/s\n", f -> v_s);
    printf("Horizontal v:\t\t%g m/s\n", f -> vh);
    printf("Velocity:\t\t%g m/s\n", f -> v);
    printf("\n");
    printf("Atmo press:\t\t%g kPa\n", f -> p/1000);
    printf("Drag:\t\t\t%g N\n", f -> D);
    printf("Drag a:\t\t\t%g m/s²\n", f -> ad);
    printf("Gravity:\t\t%g m/s²\n", f -> g);
    printf("Centrifugal a:\t\t%g m/s²\n", f -> ac);
    printf("Balanced a:\t\t%g m/s²\n", f -> ab);
    printf("Vertical a:\t\t%g m/s²\n", f -> av);
    printf("Horizontal a:\t\t%g m/s²\n", f -> ah);
    printf("Radius:\t\t\t%g km\n", f -> r/1000);
    printf("Apoapsis:\t\t%g km\n", f -> Ap/1000);
    printf("Distance Downrange\t%g km\n", f -> s/1000);
    printf("______________________\n\n");
}

// ------------------------------------------------------------

void launch_calculator() {
    struct LV lv;

    int selection = 0;
    char name[30] = "Test";
    char title[] = "LAUNCH CALCULATOR:";
    char options[] = "Go Back; Calculate; Choose Profile; Create new Profile; Testing";
    char question[] = "Program: ";
    do {
        selection = user_selection(title, options, question);

        switch(selection) {
            case 1:
                if(lv.stages != NULL) calculate_launch(lv); // if lv initialized
                break;
            case 2:
                read_LV_from_file(&lv);
                break;
            case 3:
                create_new_Profile();
                break;
            case 4:
                get_test_LV(&lv);
                calculate_launch(lv);
                break;
        }
    } while(selection != 0);
}

void calculate_launch(struct LV lv) {
    struct Vessel vessel = init_vessel();
    struct Body earth = EARTH();
    struct Flight flight = init_flight(&earth, 28.6);

    double *flight_data = (double*) calloc(1, sizeof(double));
    flight_data[0] = 1;    // amount of data points

    for(int i = 0; i < lv.stage_n; i++) {
        printf("STAGE %d:\t", i+1);
        init_vessel_next_stage(&vessel, lv.stages[i].F_sl, lv.stages[i].F_vac, lv.stages[i].m0, lv.stages[i].burn_rate);
        double burn_duration = (lv.stages[i].m0-lv.stages[i].me) / lv.stages[i].burn_rate;
        flight_data = calculate_stage_flight(&vessel, &flight, burn_duration, lv.stage_n, flight_data);
        vessel.dV += calculate_dV(vessel.F, vessel.m0, burn_duration, vessel.burn_rate);
    }

    printf("Coast:\t\t");
    init_vessel_next_stage(&vessel, 0, 0, lv.stages[lv.stage_n-1].me, 0);
    double duration = flight.vv/flight.ab + sqrt( pow(flight.vv/flight.ab,2) + 2*(flight.h-80e3)/flight.ab);
    flight_data = calculate_stage_flight(&vessel, &flight, duration, lv.stage_n, flight_data);



    char pcsv;
    printf("Write data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if(pcsv == 'y' || pcsv == 'Y') {
        char flight_data_fields[] = "Time,Thrust,Mass,Pitch,VessAcceleration,AtmoPress,Drag,DragA,HorizontalA,Gravity,CentrifugalA,BalancedA,VerticalA,HorizontalV,VerticalV,Velocity,Altitude,Apoapsis";
        write_csv(flight_data_fields, flight_data);
    }

    free(flight_data);

    print_vessel_info(&vessel);
    print_flight_info(&flight);
}

double calculate_dV(double F, double m0, double t, double burn_rate) {
    return -(F*log(m0-burn_rate*t))/burn_rate + (F*log(m0))/burn_rate;
}

double * calculate_stage_flight(struct Vessel *v, struct Flight *f, double T, int number_of_stages, double *flight_data) {
    double t;
    double step = 0.01;

    struct Vessel v_last;
    struct Flight f_last;

    start_stage(v, f);
    v_last = *v;
    f_last = *f;
    store_flight_data(v, f, &flight_data);
    
    printf("% 3d%%", 0);

    for(t = 0; t <= T-step; t += step) {
        update_flight(v,&v_last, f, &f_last, t, step);
        f -> Ap   = calc_Apoapsis(*f);
        double x = remainder(t,(T/(888/number_of_stages+1)));   // only store 890 (888 in this loop) data points overall
        if(x < step && x >=0) {
            store_flight_data(v, f, &flight_data);
        }
        v_last = *v;
        f_last = *f;

        printf("\b\b\b\b");
        printf("% 3d%%", (int)(t*100/T));
    }
    update_flight(v,&v_last, f, &f_last, t, T-t);
    f -> Ap   = calc_Apoapsis(*f);
    store_flight_data(v, f, &flight_data);

    printf("\b\b\b\b\b");
    printf("% 3d%%\n", 100);
    return flight_data;
}



void start_stage(struct Vessel *v, struct Flight *f) {
    f -> p   = get_atmo_press(f->h, f->body->scale_height);
    update_vessel(v, 0, f->p, f->h);
    f -> r   = f->h + f->body->radius;
    f -> v   = calc_velocity(f->vh,f->vv);
    f -> v_s = calc_velocity(f->vh_s, f->vv);
    f -> D   = calc_aerodynamic_drag(f->p, f->v);
    f -> ad  = f->D/v->mass;
    f -> ah  = calc_horizontal_acceleration(v->ah, f->ad, f->vh_s, f->v_s);
    f -> ac  = calc_centrifugal_acceleration(f);
    f -> g   = calc_grav_acceleration(f);
    f -> ab  = calc_balanced_acceleration(f->g, f->ac);
    f -> av  = calc_vertical_acceleration(v->av, f->ab, f->ad, f->vv, f->v_s);
    update_orbit_param(&f->orbit, f->r, f->vv, f->vh);
    calc_pos_in_orbit(f);
}


void update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double t, double step) {
    f -> t   += step;
    f -> p    = get_atmo_press(f->h, f->body->scale_height);
    update_vessel(v, t, f->p, f->h);
    calc_pos_in_orbit(f);
    f -> D    = calc_aerodynamic_drag(f->p, f->v_s);
    f -> ad   = f->D/v->mass;
    f -> ah   = calc_horizontal_acceleration(v->ah, f->ad, f->vh_s, f->v_s);
    f -> vh  += integrate(f->ah,last_f->ah,step);    // integrate horizontal acceleration
    f -> vh_s+= integrate(f->ah,last_f->ah,step);    // integrate horizontal acceleration
    f -> ac   = calc_centrifugal_acceleration(f);
    f -> g    = calc_grav_acceleration(f);
    f -> ab   = calc_balanced_acceleration(f->g, f->ac);
    f -> av   = calc_vertical_acceleration(v->av, f->ab, f->ad, f->vv, f->v_s);
    f -> vv  += integrate(f->av,last_f->av,step);    // integrate vertical acceleration
    f -> v    = calc_velocity(f->vh,f->vv);
    f -> v_s  = calc_velocity(f->vh_s, f->vv);
    f -> h   += integrate(f->vv,last_f->vv,step);    // integrate vertical speed
    f -> r    = f->h + f->body->radius;
    f -> s   += integrate(f->vh_s, last_f->vh_s, step);
    update_orbit_param(&f->orbit, f->r, f->vv, f->vh);
}

void calc_pos_in_orbit(struct Flight *f) {
    calc_true_anomaly(&f->orbit, f->r, f->vv);
    f->vh = calc_horizontal_speed(&f->orbit, f->r, f->v);
    f->vv = sqrt(pow(f->v, 2) - pow(f->vh, 2));
    if(f->orbit.theta > M_PI) f->vv *= (-1);
}


void update_vessel(struct Vessel *v, double t, double p, double h) {
    v -> F = get_thrust(v->F_vac, v->F_sl, p);
    v -> mass = v->m0 - v->burn_rate*t;     // m(t) = m0 - br*t
    v -> pitch = get_pitch(h);
    v -> a  = v->F / v->mass;       // a = F/m
    v -> ah = v->a * cos(deg_to_rad(v->pitch));
    v -> av = v->a * sin(deg_to_rad(v->pitch));
    return;
}



double get_atmo_press(double h, double scale_height) {
    if(h<140e3) return 101325*exp(-(1/scale_height) * h);
    else return 0;
}

double calc_aerodynamic_drag(double p, double v) {
    return 0.5*(p)*pow(v,2) * 7e-5;    // constant by good guess
}

double get_thrust(double F_vac, double F_sl, double p) {
    return F_vac + (p/101325)*(F_sl-F_vac);    // sea level pressure ~= 101325 Pa
}

double get_pitch(double h) {
    //return (9.0/4000.0)*pow(t,2) - 0.9*t + 90.0;
    //return 90.0-(90.0/200.0)*t;
    if(h < 38108) return 90.0*exp(-0.00003*h);
    else return 42.0*exp(-0.00001*h);
}



double calc_centrifugal_acceleration(struct Flight *f) {
    return pow(f->vh, 2) / f->r;
}

double calc_grav_acceleration(struct Flight *f) {
    return f->body->mu / pow(f->r,2);
}

double calc_balanced_acceleration(double g, double centri_a) {
    return g - centri_a;
}

double calc_vertical_acceleration(double vertical_a_thrust, double balanced_a, double drag_a, double vert_speed, double v) {
    double vertical_drag_a = 0;
    if(v!=0) vertical_drag_a = drag_a*(vert_speed/v);
    return vertical_a_thrust - balanced_a - vertical_drag_a;
}

double calc_horizontal_acceleration(double horizontal_a_thrust, double drag_a,  double hor_speed, double v) {
    double horizontal_drag_a = 0;
    if(v!=0) horizontal_drag_a = drag_a*(hor_speed/v);
    return horizontal_a_thrust - horizontal_drag_a;
}

double calc_velocity(double vh, double vv) {
    return sqrt(vv*vv+vh*vh);
}

double calc_Apoapsis(struct Flight f) {
    struct Vector r  = {0,f.r};         // current position of vessel
    struct Vector v  = {f.vh, f.vv};    // velocity vector of vessel
    double a = (f.body->mu*f.r) / (2*f.body->mu - f.r*pow(f.v,2));
    double h = cross_product(r,v);  // angular momentum
    struct Vector e;                // eccentricity vector
    e.x = r.x/vector_magnitude(r) - (h*v.y) / f.body->mu;
    e.y = r.y/vector_magnitude(r) + (h*v.x) / f.body->mu;
    double Ap = a*(1+vector_magnitude(e)) - f.body->radius;
    return Ap;
}


double integrate(double fa, double fb, double step) {
    return ( (fa+fb)/2 ) * step;
}

double deg_to_rad(double deg) {
    return deg*(M_PI/180);
}





void store_flight_data(struct Vessel *v, struct Flight *f, double **data) {
    int initial_length = (int)*data[0];
    int n_param = 18;   // number of saved parameters
    data[0] = (double*) realloc(*data, (initial_length+n_param)*sizeof(double));
    data[0][initial_length+0] = f->t;
    data[0][initial_length+1] = v->F;
    data[0][initial_length+2] = v->mass;
    data[0][initial_length+3] = v->pitch;
    data[0][initial_length+4] = v->a;
    data[0][initial_length+5] = f->p;
    data[0][initial_length+6] = f->D;
    data[0][initial_length+7] = f->ad;
    data[0][initial_length+8] = f->ah;
    data[0][initial_length+9] = f->g;
    data[0][initial_length+10]= f->ac;
    data[0][initial_length+11]= f->ab;
    data[0][initial_length+12]= f->av;
    data[0][initial_length+13]= f->vh;
    data[0][initial_length+14]= f->vv;
    data[0][initial_length+15]= f->v;
    data[0][initial_length+16]= f->h;
    data[0][initial_length+17]= f->Ap;
    data[0][0] += n_param;
    return;
}