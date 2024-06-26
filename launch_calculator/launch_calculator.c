#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "launch_calculator.h"
#include "launch_circularization.h"


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
    double A;           // cross-section (relevant for drag) [m²]
    double c_d;         // drag coefficient
    struct Lp_Params lp_param; // launch profile parameters (a1, a2, b2)
    enum STATUS status; // ACS (Ascending stage), CIRC (Circularization stage, COAST (Coasting)
};

struct Flight {
    struct Body *body;
    double t;       // time passed since t0 [s]
    double p;       // atmospheric pressure [Pa]
    double D;       // atmospheric Drag [N]
    double ad;      // acceleration due to aerodynamic drag [m/s²]
    double ah;      // current horizontal acceleration due to thrust and with drag [m/s²]
    double g;       // gravitational acceleration [m/s²]
    double av;      // current vertical acceleration due to Thrust, pitch, gravity and velocity [m/s²]
    double vh_s;    // horizontal surface speed [m/s]
    double vh;      // horizontal orbital speed [m/s]
    double vv;      // vertical speed [m/s]
    double v_s;     // overall surface velocity [m/s]
    double v;       // overall orbital velocity [m/s]
    double h;       // altitude above sea level [m]
    double r;       // distance to center of body [m]
    double a;       // semi-major axis of orbit [m]
    double e;       // eccentricity of orbit
    double i;       // inclination during flight [°]
};

struct Vessel init_vessel(struct Lp_Params lp_param, double A, double c_d) {
    struct Vessel new_vessel;
    new_vessel.mass = 0;
    new_vessel.pitch = 90;
    new_vessel.a = 0;
    new_vessel.ah = 0;
    new_vessel.av = 0;
    new_vessel.dV = 0;
    new_vessel.lp_param = lp_param;
    new_vessel.A = A;
    new_vessel.c_d = c_d;
    return new_vessel;
}

void init_vessel_next_stage(struct Vessel *vessel, double F_sl, double F_vac, double m0, double br, enum STATUS status) {
    vessel -> F_vac = F_vac;
    vessel -> F_sl = F_sl;
    vessel -> m0 = m0;
    vessel -> burn_rate = br;
    vessel -> status = status;
}

struct Flight init_flight(struct Body *body, double latitude) {
    struct Flight new_flight;
    new_flight.body = body;
    new_flight.t = 0;
    new_flight.i = latitude;
    new_flight.vh_s = 0;
    // horizontal surface speed of body at the equator -> circumference divided by rotational period [m/s]
    double vh_at_equator = (2*body->radius*M_PI) / body->rotation_period;
    new_flight.vh = vh_at_equator*cos(deg_to_rad(latitude));
    new_flight.vv = 0;
    new_flight.v_s = 0;
    new_flight.v = calc_velocity(new_flight.vv, new_flight.vh);
    new_flight.h = 0;
    new_flight.r = new_flight.h + new_flight.body->radius;
    return new_flight;
}


void print_vessel_info(struct Vessel *v) {
    printf("\n______________________\nVESSEL:\n\n");
    printf("Thrust:\t\t%g kN\n", v -> F/1000);
    printf("Mass:\t\t%g kg\n", v -> mass);
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
    printf("Vertical a:\t\t%g m/s²\n", f -> av);
    printf("Horizontal a:\t\t%g m/s²\n", f -> ah);
    printf("Radius:\t\t\t%g km\n", f -> r/1000);
    printf("Apoapsis:\t\t%g km\n", calc_apoapsis(*f) / 1000);
    printf("Periapsis:\t\t%g km\n", calc_periapsis(*f) / 1000);
    printf("______________________\n\n");
}

// ------------------------------------------------------------

void calc_launch_azimuth(struct Body *body) {
    double lat = 0;
    double incl = 0;

    printf("Enter parameters (latitude, inclination): ");
    scanf("%lf, %lf", &lat, &incl);

    double surf_speed = cos(deg_to_rad(lat)) * body->radius*2*M_PI/body->rotation_period;
    double end_speed = 7800;    // orbital speed hard coded


    double azi1 = asin((cos(deg_to_rad(incl)))/cos(deg_to_rad(lat)));
    double azi2 = asin(surf_speed/end_speed);
    double azi = rad_to_deg(azi1)-rad_to_deg(azi2);

    printf("\nNeeded launch Azimuth to target inclination %g° from %g° latitude: %g° %g°\n____________\n\n", incl, lat, azi, 90-azi+90);
}

// ------------------------------------------------------------

void launch_calculator() {
    struct LV lv;

    int selection;
    char title[] = "LAUNCH CALCULATOR:";
    char options[] = "Go Back; Calculate; Choose Profile; Create new Profile; Testing; Launch Profile Adjustments; Payload Launch Profile Analysis; Calculate Launch Azimuth";
    char question[] = "Program: ";
    do {
        selection = user_selection(title, options, question);

        switch(selection) {
            case 1:
                if(lv.stages != NULL) initiate_launch_campaign(lv, 0); // if lv initialized
                break;
            case 2:
                read_LV_from_file(&lv);
                break;
            case 3:
                write_temp_LV_file();
                break;
            case 4:
                get_test_LV(&lv);
                initiate_launch_campaign(lv, 0);
                break;
            case 5:
                if(lv.stages != NULL) initiate_launch_campaign(lv, 1); // if lv initialized
                break;
            case 6:
                if(lv.stages != NULL) initiate_launch_campaign(lv, 2); // if lv initialized
                break;
            case 7:
                calc_launch_azimuth(EARTH());
                break;
        }
    } while(selection != 0);
}

void initiate_launch_campaign(struct LV lv, int calc_params) {
    if(calc_params != 0) {
        if(calc_params == 1) {
            double payload_mass = 50;
            struct Lp_Params best_lp_params;
            lp_param_fixed_payload_analysis(lv, payload_mass, &best_lp_params);
            printf("\n------- a1: %f, a2: %f, b2: %g, h: %g ------- \n\n", best_lp_params.a1, best_lp_params.a2, best_lp_params.b2, best_lp_params.h);
            calculate_launch(lv, payload_mass, best_lp_params, 0);
        } else {
            double payload_max = calc_highest_payload_mass(lv);
            printf("Payload max: %g\n", payload_max);
            lp_param_mass_analysis(lv, 285, payload_max);
            lp_param_mass_analysis(lv, 50, 280);
            lp_param_mass_analysis(lv, 10, 50);
            lp_param_mass_analysis(lv, 0, 10);
        }
    } else {
        //struct Lp_Params lp_params = {.a1 = 36e-6, .a2 = 12e-6, .b2 = 49};    // tester
        struct Lp_Params lp_params = {.a1 = 30e-6, .a2 = 6e-6, .b2 = 58};      // electron
        //struct Lp_Params lp_params = {.a1 = 0e-6, .a2 = 0e-6, .b2 = 90};      // sounding
        if(1) lp_params.h = log(lp_params.b2/90) / (lp_params.a2-lp_params.a1);
        else lp_params.h = 1e9;
        double payload_mass = 300;
        calculate_launch(lv, payload_mass, lp_params, 0);
    }
}

struct Launch_Results calculate_launch(struct LV lv, double payload_mass, struct Lp_Params lp_param, int calc_params) {
    struct timeval start_time, end_time;
    double elapsed_time;
    gettimeofday(&start_time, NULL);

    struct Vessel vessel = init_vessel(lp_param, lv.A, lv.c_d);
    struct Body *earth = EARTH();
    // struct Flight flight = init_flight(earth, 28.6); // KSC
    struct Flight flight = init_flight(earth, 39.26); // Mahia
    //struct Flight flight = init_flight(earth, 90); // Pole

    double *flight_data = (double*) calloc(1, sizeof(double));
    flight_data[0] = 1;    // amount of data points

    double left_over_propellant;
    double left_over_dv;
    for(int i = 0; i < lv.stage_n; i++) {
        // Separation (not for first stage) - further adjustments for side booster in the future...
        // printf("Stage %d:\n", i+1);
        if(i > 0) {
            // after engine burnout and before separation
            init_vessel_next_stage(&vessel, 0, 0, vessel.mass, 0, ASC);
            flight_data = calculate_stage_flight(&vessel, &flight, 4, lv.stage_n, flight_data);
            // after separation and before engine ignition
            init_vessel_next_stage(&vessel, 0, 0, lv.stages[i].m0+payload_mass, 0, ASC);
            flight_data = calculate_stage_flight(&vessel, &flight, 4, lv.stage_n, flight_data);
        }

        double burn_duration = (lv.stages[i].m0-lv.stages[i].me) / lv.stages[i].burn_rate;
        // Circularization stage, if stage could get approximately to orbit
        enum STATUS status = (vessel.dV+ calculate_dV(lv.stages[i].F_vac, lv.stages[i].m0+payload_mass, lv.stages[i].m0+payload_mass - burn_duration*lv.stages[i].burn_rate, lv.stages[i].burn_rate)) < 7800 ?
                             ASC : CIRC;
        init_vessel_next_stage(&vessel, lv.stages[i].F_sl, lv.stages[i].F_vac, lv.stages[i].m0 + payload_mass, lv.stages[i].burn_rate, status);
        flight_data = calculate_stage_flight(&vessel, &flight, burn_duration, lv.stage_n, flight_data);
        left_over_propellant = vessel.mass - (vessel.m0 - burn_duration*vessel.burn_rate);
        left_over_dv = calculate_dV(vessel.F_vac, vessel.mass, vessel.m0 - burn_duration*vessel.burn_rate, vessel.burn_rate);
    }

    if(!calc_params) {
        //printf("Coast:\n");
        init_vessel_next_stage(&vessel, 0, 0, vessel.mass, 0, COAST);
        double ab = flight.g - pow(flight.vh, 2) / flight.r;
        double temp = flight.vv/ab + sqrt( pow(flight.vv/ab,2) + 2*(flight.h-50e3)/ab);
        double duration = ( temp > 0 && calc_periapsis(flight) < 100e3) ? temp : 600;
        flight_data = calculate_stage_flight(&vessel, &flight, duration, lv.stage_n, flight_data);


        gettimeofday(&end_time, NULL);

        // Calculate the elapsed time in seconds
        elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                       (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

        printf("Elapsed time: %f seconds\n", elapsed_time);
        printf("Elapsed time: %f ms\n", elapsed_time*1000);


        print_vessel_info(&vessel);
        print_flight_info(&flight);

        printf("Payload: %g kg\nLeft-over delta-V: %g m/s\nLeft-over propellant: %g kg\nPossible Payload: %g kg\n",
               payload_mass, left_over_dv, left_over_propellant, payload_mass + left_over_propellant);

        char pcsv;
        printf("Write data to .csv (y/Y=yes)? ");
        scanf(" %c", &pcsv);
        if (pcsv == 'y' || pcsv == 'Y') {
            char flight_data_fields[] = "Time,Thrust,Mass,Pitch,AtmoPress,Drag,HorizontalV,VerticalV,OrbitalV,HorizontalSurfV,SurfaceV,Altitude,Semi-MajorAxis,Eccentricity";
            write_csv(flight_data_fields, flight_data);
        }

    }

    free(flight_data);

    struct Launch_Results results;
    results.pe = calc_periapsis(flight);
    results.dv = vessel.dV;
    results.rf = left_over_propellant;
    return results;
}

double calculate_dV(double F, double m0, double mf, double burn_rate) {
    return F/burn_rate * log(m0/mf);
}

double * calculate_stage_flight(struct Vessel *v, struct Flight *f, double T, int number_of_stages, double *flight_data) {
    double t;
    double step = 1e-2;

    struct Vessel v_last;
    struct Flight f_last;

    start_stage(v, f);
    v_last = *v;
    f_last = *f;
    store_flight_data(v, f, &flight_data);

    //printf("% 3d%%", 0);

    for(t = 0; t <= T-step; t += step) {
        update_flight(v,&v_last, f, &f_last, t, step);
        f -> a = calc_semi_major_axis(*f);
        f -> e = calc_eccentricity(*f);
        if(v->status == CIRC && f->e > f_last.e) T = t;
        double x = remainder(t,(T/(888/number_of_stages+1)));   // only store 890 (888 in this loop) data points overall
        if(x < step && x >=0) {
            store_flight_data(v, f, &flight_data);
        }
        v_last = *v;
        f_last = *f;

        //printf("\b\b\b\b");
        //printf("% 3d%%", (int)(t*100/T));
    }
    update_flight(v,&v_last, f, &f_last, t, T-t);
    f -> a = calc_semi_major_axis(*f);
    f -> e = calc_eccentricity(*f);
    store_flight_data(v, f, &flight_data);

    //printf("\b\b\b\b\b");
    //printf("% 3d%%\n", 100);
    return flight_data;
}



void start_stage(struct Vessel *v, struct Flight *f) {
    f -> p   = get_atmo_press(f->h, f->body->scale_height);
    update_vessel(v, 0, f->p);
    f -> r   = f->h + f->body->radius;
    f -> v   = calc_velocity(f->vh,f->vv);
    f -> v_s = calc_velocity(f->vh_s, f->vv);
    f -> D   = calc_aerodynamic_drag(f->p, f->v_s, v->A, v->c_d);
    f -> ad  = f->D/v->mass;
    f -> ah  = calc_horizontal_acceleration(v->ah, f->ad, f->vh_s, f->v_s);
    f -> g   = calc_grav_acceleration(f);
    f -> av  = calc_vertical_acceleration(v->av, *f);
    f -> a   = calc_semi_major_axis(*f);
    f -> e   = calc_eccentricity(*f);
}


void update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double t, double step) {
    if(step == 0) return;
    f -> t   += step;
    f -> p    = get_atmo_press(f->h, f->body->scale_height);
    v -> pitch = get_pitch(*v, *f);
    update_vessel(v, t, f->p);
    v -> dV  += integrate(v->a, last_v->a, step);
    f -> D    = calc_aerodynamic_drag(f->p, f->v_s, v->A, v->c_d);
    f -> ad   = f->D/v->mass;
    f -> ah   = calc_horizontal_acceleration(v->ah, f->ad, f->vh_s, f->v_s);
    f -> vh  += integrate(f->ah,last_f->ah,step);    // integrate horizontal acceleration
    f -> vh_s+= integrate(f->ah,last_f->ah,step);    // integrate horizontal acceleration
    f -> g    = calc_grav_acceleration(f);
    f -> av   = calc_vertical_acceleration(v->av, *f);
    f -> vv  += integrate(f->av,last_f->av,step);    // integrate vertical acceleration
    f -> v    = calc_velocity(f->vh,f->vv);
    f -> v_s  = calc_velocity(f->vh_s, f->vv);
    f -> h   += integrate(f->vv,last_f->vv,step);    // integrate vertical speed
    f -> r    = f->h + f->body->radius;
    // change of frame of reference (vv already changed due to centrifugal force in av calc)
    calc_change_of_reference_frame(f, last_f, step);
}


void update_vessel(struct Vessel *v, double t, double p) {
    v -> F = get_thrust(v->F_vac, v->F_sl, p);
    v -> mass = v->m0 - v->burn_rate*t;     // m(t) = m0 - br*t
    v -> a  = v->F / v->mass;       // a = F/m
    v -> ah = v->a * cos(deg_to_rad(v->pitch));
    v -> av = v->a * sin(deg_to_rad(v->pitch));
    return;
}



double get_atmo_press(double h, double scale_height) {
    if(h<140e3) return 101325*exp(-h/scale_height);
    else return 0;
}

double calc_aerodynamic_drag(double p, double v, double A, double c_d) {
    double rho = p/57411.6;
    return 0.5*rho*A*c_d*pow(v,2);
}

double get_thrust(double F_vac, double F_sl, double p) {
    return F_vac + (p/101325)*(F_sl-F_vac);    // sea level pressure ~= 101325 Pa
}

double get_pitch(struct Vessel v, struct Flight f) {
    switch(v.status) {
        case ASC: {
            double a1 = v.lp_param.a1;
            double a2 = v.lp_param.a2;
            double b2 = v.lp_param.b2;
            if (f.h < v.lp_param.h) return 90.0 * exp(-a1 * f.h);
            else return b2 * exp(-a2 * f.h); }
        case CIRC:
            return get_circularization_pitch(
                    v.F, v.mass, v.burn_rate, f.vh, f.vv,
                    calc_apoapsis(f) + f.body->radius, f.body->mu);
        case COAST:
            return 0;
    }
}


double calc_grav_acceleration(struct Flight *f) {
    //return 9.81;
    return f->body->mu / pow(f->r,2);
}

double calc_vertical_acceleration(double a_vertical_thrust, struct Flight f) {
    double a_centrifugal = pow(f.vh, 2) / f.r;     // centrifugal acceleration
    double a_vertical_drag = f.v_s != 0 ? f.ad*(f.vv/f.v_s) : 0;
    return a_vertical_thrust - (f.g - a_centrifugal) - a_vertical_drag;
}

double calc_horizontal_acceleration(double a_horizontal_thrust, double a_drag, double vh_s, double v_s) {
    double a_horizontal_drag = v_s != 0 ? a_drag*(vh_s/v_s) : 0;
    return a_horizontal_thrust - a_horizontal_drag;
}

double calc_velocity(double vh, double vv) {
    return sqrt(vv*vv+vh*vh);
}

// calculates new horizontal speed in new frame of reference (vertical speed not needed to be recalculated, as flat earth is assumed)
void calc_change_of_reference_frame(struct Flight *f, struct Flight *last_f, double step) {
    double dx = integrate(f->vh, last_f->vh, step);
    f->vh = fabs((1/f->r)*(dx*f->vv-sqrt(pow(f->r,2)-dx*dx)*f->vh));
    f->vh_s = fabs((1/f->r)*(dx*f->vv-sqrt(pow(f->r,2)-dx*dx)*f->vh_s));
}

double calc_semi_major_axis(struct Flight f) {
    return (f.body->mu*f.r) / (2*f.body->mu - f.r*pow(f.v,2));
}

double calc_eccentricity(struct Flight f) {
    double h = -f.r * f.vh;         // angular momentum (cross product of r and v; r_x = 0)
    return sqrt(1-(h*h)/(f.body->mu * f.a));
}

double calc_apoapsis(struct Flight f) {
    return f.a*(1+f.e) - f.body->radius;
}

double calc_periapsis(struct Flight f) {
    return f.a*(1-f.e) - f.body->radius;
}

double integrate(double fa, double fb, double step) {
    return ( (fa+fb)/2 ) * step;
}

double deg_to_rad(double deg) {
    return deg*(M_PI/180);
}

double rad_to_deg(double rad) {
    return rad/(M_PI/180);
}





void store_flight_data(struct Vessel *v, struct Flight *f, double **data) {
    int initial_length = (int)*data[0];
    int n_param = 14;   // number of saved parameters
    data[0] = (double*) realloc(*data, (initial_length+n_param)*sizeof(double));
    data[0][initial_length+0] = f->t;
    data[0][initial_length+1] = v->F;
    data[0][initial_length+2] = v->mass;
    data[0][initial_length+3] = v->pitch;
    data[0][initial_length+4] = f->p;
    data[0][initial_length+5] = f->D;
    data[0][initial_length+6] = f->vh;
    data[0][initial_length+7] = f->vv;
    data[0][initial_length+8] = f->v;
    data[0][initial_length+9] = f->vh_s;
    data[0][initial_length+10]= f->v_s;
    data[0][initial_length+11]= f->h;
    data[0][initial_length+12]= f->a;
    data[0][initial_length+13]= f->e;
    data[0][0] += n_param;
    return;
}