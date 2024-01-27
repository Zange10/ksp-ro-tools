#include "transfer_calculator.h"
#include "transfer_tools.h"
#include "double_swing_by.h"
#include "csv_writer.h"
#include "tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

enum Transfer_Type final_tt = circcap;

void simple_transfer() {
    struct Body *bodies[2] = {EARTH(), JUPITER()};
    struct Date min_dep_date = {1977, 1, 1, 0, 0, 0};
    struct Date max_dep_date = {1978, 1, 1, 0, 0, 0};
    int min_duration = 400;         // [days]
    int max_duration = 1500;         // [days]
    double dep_time_steps = 24 * 60 * 60; // [seconds]
    double arr_time_steps = 24 * 60 * 60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    double jd_max_arr = jd_max_dep + max_duration;

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(jd_max_arr - jd_min_dep) / ephem_time_steps + 1;

    struct Ephem **ephems = (struct Ephem**) malloc(2*sizeof(struct Ephem*));
    for(int i = 0; i < 2; i++) {
        ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
        get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_min_dep, jd_max_arr, 1);
    }

    struct Porkchop_Properties pochopro = {
            jd_min_dep,
            jd_max_dep,
            dep_time_steps,
            arr_time_steps,
            min_duration,
            max_duration,
            ephems,
            bodies[0],
            bodies[1]
    };
    // 4 = dep_time, duration, dv1, dv2
    int data_length = 4;
    int all_data_size = (int) (data_length * (max_duration - min_duration) / (arr_time_steps / (24 * 60 * 60)) *
                               (jd_max_dep - jd_min_dep) / (dep_time_steps / (24 * 60 * 60))) + 1;
    double *porkchop = (double *) malloc(all_data_size * sizeof(double));

    create_porkchop(pochopro, circcirc, porkchop);
    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
    write_csv(data_fields, porkchop);

    int mind = 0;
    double min = 1e9;

    for(int i = 0; i < porkchop[0]/4; i++) {
        if(porkchop[i*4+4] < min) {
            mind = i;
            min = porkchop[i*4+4];
        }
    }

    double t_dep = porkchop[mind*4+1];
    double t_arr = t_dep + porkchop[mind*4+2];

    free(porkchop);

    struct OSV s0 = osv_from_ephem(ephems[0], t_dep, SUN());
    struct OSV s1 = osv_from_ephem(ephems[1], t_arr, SUN());

    double data[3];
    struct Transfer transfer = calc_transfer(circcirc, bodies[0], bodies[1], s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24*60*60), data);
    printf("Departure: ");
    print_date(convert_JD_date(t_dep),0);
    printf(", Arrival: ");
    print_date(convert_JD_date(t_arr),0);
    printf(" (%f days), Delta-v: %f m/s (%f m/s, %f m/s)\n",
           t_arr-t_dep, data[1]+data[2], data[1], data[2]);

    int num_states = 4;

    double times[] = {t_dep, t_dep, t_arr, t_arr};
    struct OSV osvs[num_states];
    osvs[0] = s0;
    osvs[1].r = transfer.r0;
    osvs[1].v = transfer.v0;
    osvs[2].r = transfer.r1;
    osvs[2].v = transfer.v1;
    osvs[3] = s1;

    double transfer_data[num_states*7+1];
    transfer_data[0] = 0;

    for(int i = 0; i < num_states; i++) {
        transfer_data[i*7+1] = times[i];
        transfer_data[i*7+2] = osvs[i].r.x;
        transfer_data[i*7+3] = osvs[i].r.y;
        transfer_data[i*7+4] = osvs[i].r.z;
        transfer_data[i*7+5] = osvs[i].v.x;
        transfer_data[i*7+6] = osvs[i].v.y;
        transfer_data[i*7+7] = osvs[i].v.z;
        transfer_data[0] += 7;
    }

    free(ephems[0]);
    free(ephems[1]);
    free(ephems);

    char pcsv;
    printf("Write transfer data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if (pcsv == 'y' || pcsv == 'Y') {
        char transfer_data_fields[] = "JD,X,Y,Z,VX,VY,VZ";
        write_csv(transfer_data_fields, transfer_data);
    }

}

void dsb_test() {
	struct timeval start, end;
	double elapsed_time;
	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}

	struct Body *Galileo_bodies[] = {VENUS(), EARTH(), EARTH(), JUPITER()};
	struct Date Galileo[4]= {
			{1990, 2, 10, 0, 0, 0},
			{1990, 12, 8,0,0,0},
			{1992, 12, 8,0,0,0},
			{1995, 12, 7,0,0,0},
	};

	struct Body *Cassini_bodies[] = {EARTH(), VENUS(), VENUS(), EARTH()};
	struct Date Cassini[4]= {
			{1997, 10, 15, 0, 0, 0},
			{1998, 4, 26,0,0,0},
			{1999, 6, 24,0,0,0},
			{1999, 8, 18,0,0,0},
	};
	
	struct Body **bodies = Cassini_bodies;
	struct Date *dates = Cassini;
	struct Date dep_date = dates[0];
	struct Date sb1_date = dates[1];
	struct Date sb2_date = dates[2];
	struct Date arr_date = dates[3];

	double jd_dep = convert_date_JD(dep_date);
	double jd_sb1 = convert_date_JD(sb1_date);
	double jd_sb2 = convert_date_JD(sb2_date);
	double jd_arr = convert_date_JD(arr_date);

	double durations[] = {jd_sb1-jd_dep, jd_sb2-jd_sb1, jd_arr-jd_sb2};
	
	struct OSV osv_dep = osv_from_ephem(ephems[bodies[0]->id-1], jd_dep, SUN());
	struct OSV osv_sb1 = osv_from_ephem(ephems[bodies[1]->id-1], jd_sb1, SUN());
	struct OSV osv_sb2 = osv_from_ephem(ephems[bodies[2]->id-1], jd_sb2, SUN());
	struct OSV osv_arr = osv_from_ephem(ephems[bodies[3]->id-1], jd_arr, SUN());
	
	struct Transfer transfer_dep = calc_transfer(circfb, EARTH(), VENUS(), osv_dep.r, osv_dep.v, osv_sb1.r, osv_sb1.v, (jd_sb1-jd_dep)*86400, NULL);
	struct Transfer transfer_arr = calc_transfer(circfb, VENUS(), EARTH(), osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, NULL);
	
	struct OSV s0 = {transfer_dep.r1, transfer_dep.v1};
	struct OSV s1 = {transfer_arr.r0, transfer_arr.v0};
	
	
	gettimeofday(&start, NULL);  // Record the ending time
	struct DSB dsb = calc_double_swing_by(s0, osv_sb1, s1, osv_sb2, durations[1], bodies[1]);
	
	gettimeofday(&end, NULL);  // Record the ending time
	elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	printf("\n----- | Total elapsed time: %.3f s | ---------\n", elapsed_time);
	
	for(int i = 0; i < num_bodies; i++) {
		free(ephems[i]);
	}
	free(ephems);
}

void create_swing_by_transfer() {
    struct timeval start, end;
    double elapsed_time;


    struct Body *bodies[] = {EARTH(), VENUS(), VENUS(), EARTH(), JUPITER()};
    int num_bodies = (int) (sizeof(bodies)/sizeof(struct Body*));

    //struct Date min_dep_date = {1970, 1, 1, 0, 0, 0};
    //struct Date max_dep_date = {1971, 1, 1, 0, 0, 0};
    //int min_duration[] = {60, 200};//, 1800, 1500};         // [days]
    //int max_duration[] = {250, 500};//, 2600, 2200};         // [days]
    struct Date min_dep_date = {1997, 10, 1, 0, 0, 0};
    struct Date max_dep_date = {1997, 11, 1, 0, 0, 0};
    int min_duration[] = {90, 410, 90, 700};
    int max_duration[] = {250, 430, 150, 900};

    double dep_time_steps = 24 * 60 * 60; // [seconds]
    double arr_time_steps = 24 * 60 * 60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    int max_total_dur = 0;
    for(int i = 0; i < num_bodies-1; i++) max_total_dur += max_duration[i];
    double jd_max_arr = jd_max_dep + max_total_dur;

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(jd_max_arr - jd_min_dep) / ephem_time_steps + 1;

    struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
    for(int i = 0; i < num_bodies; i++) {
        int ephem_available = 0;
        for(int j = 0; j < i; j++) {
            if(bodies[i] == bodies[j]) {
                ephems[i] = ephems[j];
                ephem_available = 1;
                break;
            }
        }
        if(ephem_available) continue;
        ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
        get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_min_dep, jd_max_arr, 1);
    }

/*
    int c = 0;

    int u = 0;
    int g = 0;

    double x1[100000];
    double x2[100000];
    double x3[100000];
    double x4[100000];
    double x5[100000];
    double x6[10];
    double y1[20000];
    double y[100000];
    double *xs[] = {x1,x2,x3,x4,x5,x6,y,y1};
    int *ints[] = {&c,&u,&g};
    int max_i = 4;
    int us[max_i];

    double a1[500];
    double a2[500];
    double a3[500];
    double a4[500];
    double a5[500];
    double a6[500];


    if(0) {
        bodies[0] = EARTH();
        bodies[1] = VENUS();
        bodies[2] = EARTH();

        struct Date dep_date = {1997, 10, 15, 0, 0, 0};
        struct Date sb1_date = {1998, 04, 26, 0, 0, 0};
        struct Date sb2_date = {1999, 06, 24, 0, 0, 0};
        struct Date arr_date = {1999, 8, 18, 0, 0, 0};

        double jd_dep = convert_date_JD(dep_date);
        double jd_sb1 = convert_date_JD(sb1_date);
        double jd_sb2 = convert_date_JD(sb2_date);
        double jd_arr = convert_date_JD(arr_date);

        for(int i = 0; i < num_bodies; i++) {
            int ephem_available = 0;
            for(int j = 0; j < i; j++) {
                if(bodies[i] == bodies[j]) {
                    ephems[i] = ephems[j];
                    ephem_available = 1;
                    break;
                }
            }
            if(ephem_available) continue;
            ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
            get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_dep, jd_arr, 0);
        }

        double dwb_dur = jd_sb2-jd_sb1;
        struct OSV osv_dep = osv_from_ephem(ephems[0], jd_dep, SUN());
        struct OSV osv_sb1 = osv_from_ephem(ephems[1], jd_sb1, SUN());
        struct OSV osv_sb2 = osv_from_ephem(ephems[1], jd_sb2, SUN());
        struct OSV osv_arr = osv_from_ephem(ephems[2], jd_arr, SUN());

        struct Transfer transfer_dep = calc_transfer(circfb, VENUS(), EARTH(), osv_dep.r, osv_dep.v, osv_sb1.r, osv_sb1.v, (jd_sb1-jd_dep)*86400, NULL);
        struct Transfer transfer_arr = calc_transfer(circfb, EARTH(), JUPITER(), osv_sb2.r, osv_sb2.v, osv_arr.r, osv_arr.v, (jd_arr-jd_sb2)*86400, NULL);

        struct OSV osv0 = {transfer_dep.r1, transfer_dep.v1};
        struct OSV osv1 = {transfer_arr.r0, transfer_arr.v0};


        gettimeofday(&start, NULL);  // Record the starting time
        calc_double_swing_by(osv0, osv_sb1, osv1, osv_sb2, dwb_dur, bodies[1], xs, ints, 0);
        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        printf("Elapsed time: %f seconds\n", elapsed_time);

        printf("\n%d %d\n", c, u);
        double minabs = 1e9;
        int mind;
        for (int a = 0; a < u; a++) {
            if (y[a] < minabs) {
                minabs = y[a];
                mind = a;
            }
        }
        printf("\nOrbital Period: %f\n", x1[mind]);
        printf("Dur to man: %f\n", x2[mind]);
        printf("theta: %f°\n", x3[mind]);
        printf("phi: %f°\n", x4[mind]);
        printf("Inclination: %f°\n", x5[mind]);
        printf("mindv: %f\n", y[mind]);

        printf("Targeting date: ");
        print_date(convert_JD_date(x2[mind]+jd_sb1),1);

        exit(0);
    }

if(1) {
    double dep0 = jd_min_dep + 240;
    double dwb_dur = 470;
    struct OSV e0 = osv_from_ephem(ephems[0], dep0, SUN());
    dep0 = jd_min_dep + 346;
    struct OSV p0 = osv_from_ephem(ephems[1], dep0, SUN());
    dep0 = jd_min_dep + 346 + dwb_dur;
    struct OSV p1 = osv_from_ephem(ephems[1], dep0, SUN());

    double temp_data[3];
    struct Transfer transfer = calc_transfer(circfb, EARTH(), VENUS(), e0.r, e0.v, p0.r, p0.v, 116.0 * 86400,
                                             temp_data);

    struct OSV osv0 = {transfer.r1, transfer.v1};
    struct OSV osv1 = {p1.r, rotate_vector_around_axis(scalar_multiply(p1.v, 1.2), p1.r, 0)};


    printf("\n---------\n\n\n");
    printf("%f %f %f\n\n", temp_data[0], temp_data[1], temp_data[2]);
    gettimeofday(&start, NULL);  // Record the starting time
    calc_double_swing_by(osv0, p0, osv1, p1, dwb_dur, VENUS(), xs, ints, 0);
    gettimeofday(&end, NULL);  // Record the ending time
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Elapsed time: %f seconds\n", elapsed_time);

    char zusatz[] = "a";

    if (0) {
        printf("x1%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x1[i]);
        }
        printf("]\n");
        printf("x2%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x2[i]);
        }
        printf("]\n");
        printf("x3%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x3[i]);
        }
        printf("]\n");
        printf("x4%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x4[i]);
        }
        printf("]\n");
        printf("x5%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", x5[i]);
        }
        printf("]\n");
        printf("y%s = [", zusatz);
        for (int i = 0; i < u; i++) {
            if (i != 0) printf(", ");
            printf("%f", y[i]);
        }
        printf("]\n");
    }

    printf("\n%d %d\n", c, u);
    double minabs = 1e9;
    int mind;
    for (int a = 0; a < u; a++) {
        if (y[a] < minabs) {
            minabs = y[a];
            mind = a;
        }
    }
    printf("\nOrbital Period: %f\n", x1[mind]);
    printf("Dur to man: %f\n", x2[mind]);
    printf("theta: %f°\n", x3[mind]);
    printf("phi: %f°\n", x4[mind]);
    printf("Inclination: %f°\n", x5[mind]);
    printf("mindv: %f\n", y[mind]);
    printf("\n%f\n", minabs);

} else {

    int b = 0;
    int d = 0;
    double tot_time = 0;
    double x10[10000];
    double x11[10000];
    double x12[10000];


    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                for (int l = 0; l < 5; l++) {
                    double dep0 = jd_min_dep + 200 + i * 20;
                    double first_travel = 300 + l * 20;
                    double dwb_dur = 420 + j * 20;
                    printf("\n-------------\ndep: ");
                    print_date(convert_JD_date(dep0), 0);
                    printf("   -->  ");
                    print_date(convert_JD_date(jd_min_dep + first_travel), 0);
                    printf("  (i: %d, j: %d, k: %d, l: %d)\ntransfer duration: %fd, v_factor: %f\n-------------\n", i,
                           j, k, l, dwb_dur, 0.75 + k * 0.15);


                    struct OSV e0 = osv_from_ephem(ephems[0], dep0, SUN());
                    dep0 = jd_min_dep + first_travel;
                    struct OSV p0 = osv_from_ephem(ephems[1], dep0, SUN());
                    dep0 = jd_min_dep + first_travel + dwb_dur;
                    struct OSV p1 = osv_from_ephem(ephems[1], dep0, SUN());

                    double temp_data[3];
                    struct Transfer transfer = calc_transfer(circfb, EARTH(), VENUS(), e0.r, e0.v, p0.r, p0.v,
                                                             116.0 * 86400,
                                                             temp_data);

                    struct OSV osv0 = {transfer.r1, transfer.v1};
                    struct OSV osv1 = {p1.r, rotate_vector_around_axis(scalar_multiply(p1.v, 0.8 + k * 0.15), p1.r,
                                                                       deg2rad(3))};


                    x6[0] = -100;
                    x6[1] = -100;
                    x6[2] = -100;
                    x6[3] = -100;

                    //printf("\n---------\n\n\n");
                    //printf("%f %f %f\n\n", temp_data[0], temp_data[1], temp_data[2]);
                    gettimeofday(&start, NULL);  // Record the starting time
                    int via = calc_double_swing_by(osv0, p0, osv1, p1, dwb_dur, VENUS(), xs, ints, 1);

                    if(via) {
                        calc_double_swing_by(osv0, p0, osv1, p1, dwb_dur, VENUS(), xs, ints, 0);
                        x10[d] = x6[2];
                        d++;
                    }


                    gettimeofday(&end, NULL);  // Record the ending time
                    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
                    tot_time += elapsed_time;
                    printf("Elapsed time: %f seconds\n", elapsed_time);

                    double min = 1e9;
                    int mind;
                    for (int a = 0; a < u; a++) {
                        if (y[a] < min) {
                            min = y[a];
                            mind = a;
                        }
                    }

                    //a1[b] = x1[mind];
                    //a2[b] = x2[mind];
                    //a3[b] = x3[mind];
                    //a4[b] = x4[mind];
                    //a5[b] = x5[mind];
                    //a6[b] = y[mind];


                    u = 0;
                    c = 0;
                    g = 0;
                    b++;


                    if (1) {
                        printf("x10 = [");
                        for (int m = 0; m < d; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", x10[m]);
                        }
                        printf("]\n");
                        printf("x11 = [");
                        for (int m = 0; m < d; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", x11[m]);
                        }
                        printf("]\n");
                        printf("x12 = [");
                        for (int m = 0; m < d; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", x12[m]);
                        }
                        printf("]\n");
                    }

                    if (0) {
                        printf("x1 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a1[m]);
                        }
                        printf("]\n");
                        printf("x2 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a2[m]);
                        }
                        printf("]\n");
                        printf("x3 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a3[m]);
                        }
                        printf("]\n");
                        printf("x4 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a4[m]);
                        }
                        printf("]\n");
                        printf("x5 = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a5[m]);
                        }
                        printf("]\n");
                        printf("y = [");
                        for (int m = 0; m < b; m++) {
                            if (m != 0) printf(", ");
                            printf("%f", a6[m]);
                        }
                        printf("]\n");

                        //exit(0);
                    }
                }
            }
        }
    }


    printf("\n Total time: %f seconds \nb: %d\n", tot_time, b);

}

    exit(0);*/

    double ** porkchops = (double**) malloc((num_bodies-1) * sizeof(double*));

    for(int i = 0; i < num_bodies-1; i++) {
        int is_double_sb = bodies[i] == bodies[i+1];

        double min_dep, max_dep;
        if(i == 0) {
            min_dep = jd_min_dep;
            max_dep = jd_max_dep;
        } else {
            min_dep = get_min_arr_from_porkchop(porkchops[i - 1]) + is_double_sb*min_duration[i];
            max_dep = get_max_arr_from_porkchop(porkchops[i - 1]) + is_double_sb*max_duration[i];
        }
        i += is_double_sb;

        struct Porkchop_Properties pochopro = {
                min_dep,
                max_dep,
                dep_time_steps,
                arr_time_steps,
                min_duration[i],
                max_duration[i],
                ephems+i,
                bodies[i],
                bodies[i+1]
        };
        // 4 = dep_time, duration, dv1, dv2
        int data_length = 4;
        int all_data_size = (int) (data_length * (max_duration[i] - min_duration[i]) / (arr_time_steps / (24 * 60 * 60)) *
                                   (max_dep - min_dep) / (dep_time_steps / (24 * 60 * 60))) + 1;
        porkchops[i] = (double *) malloc(all_data_size * sizeof(double));

        gettimeofday(&start, NULL);  // Record the starting time
        if(i<num_bodies-2) create_porkchop(pochopro, circcirc, porkchops[i]);
        else               create_porkchop(pochopro, final_tt, porkchops[i]);
        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // NOLINT(*-narrowing-conversions)
        printf("Elapsed time: %f seconds\n", elapsed_time);

        gettimeofday(&start, NULL);  // Record the starting time
        double min_max_dsb_duration[2];
        if(is_double_sb) {
            min_max_dsb_duration[0] = min_duration[i-1];
            min_max_dsb_duration[1] = max_duration[i-1];
        }
        decrease_porkchop_size(i, is_double_sb, porkchops, ephems, bodies, min_max_dsb_duration);
        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // NOLINT(*-narrowing-conversions)
        printf("Elapsed time: %f seconds\n", elapsed_time);


        // if no viable trajectory was found, exit...
        if(porkchops[i][0] == 0) return;
    }

    double *jd_dates = (double*) malloc(num_bodies*sizeof(double));

    int min_total_dur = 0;
    for(int i = 0; i < num_bodies-1; i++) min_total_dur += min_duration[i];

    double *final_porkchop = (double *) malloc((int)(((max_total_dur-min_total_dur)*(jd_max_dep-jd_min_dep))*4+1)*sizeof(double));
    final_porkchop[0] = 0;

    gettimeofday(&start, NULL);  // Record the starting time
    double dep_time_info[] = {(jd_max_dep-jd_min_dep)*86400/dep_time_steps+1, dep_time_steps, jd_min_dep};
    generate_final_porkchop(porkchops, num_bodies, jd_dates, final_porkchop, dep_time_info);

    show_progress("Looking for cheapest transfer", 1, 1);
    printf("\n");

    gettimeofday(&end, NULL);  // Record the ending time
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // NOLINT(*-narrowing-conversions)
    printf("Elapsed time: %f seconds\n", elapsed_time);

    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
    write_csv(data_fields, final_porkchop);

    free(final_porkchop);
    for(int i = 0; i < num_bodies-1; i++) free(porkchops[i]);
    free(porkchops);

    // 3 states per transfer (departure, arrival and final state)
    // 1 additional for initial start; 7 variables per state
    double transfer_data[((num_bodies-1)*3+1) * 7 + 1];
    transfer_data[0] = 0;

    struct OSV init_s = osv_from_ephem(ephems[0], jd_dates[0], SUN());

    transfer_data[1] = jd_dates[0];
    transfer_data[2] = init_s.r.x;
    transfer_data[3] = init_s.r.y;
    transfer_data[4] = init_s.r.z;
    transfer_data[5] = init_s.v.x;
    transfer_data[6] = init_s.v.y;
    transfer_data[7] = init_s.v.z;
    transfer_data[0] += 7;

    double dep_dv;

    for(int i = 0; i < num_bodies-1; i++) {
        struct OSV s0 = osv_from_ephem(ephems[i], jd_dates[i], SUN());
        struct OSV s1 = osv_from_ephem(ephems[i+1], jd_dates[i+1], SUN());

        double data[3];
        struct Transfer transfer;

        if(i < num_bodies-2) {
            transfer = calc_transfer(circcirc, bodies[i], bodies[i + 1], s0.r, s0.v, s1.r, s1.v,
                                     (jd_dates[i + 1] - jd_dates[i]) * (24 * 60 * 60), data);
        } else {
            transfer = calc_transfer(final_tt, bodies[i], bodies[i + 1], s0.r, s0.v, s1.r, s1.v,
                                     (jd_dates[i + 1] - jd_dates[i]) * (24 * 60 * 60), data);
        }

        if(i == 0) {
            printf("| T+    0 days (");
            print_date(convert_JD_date(jd_dates[i]),0);
            char *spacing = "                 ";
            printf(")%s- %s (%.2f m/s)\n", spacing, bodies[i]->name, data[1]);
            dep_dv = data[1];
        }
        printf("| T+% 5d days (", (int)(jd_dates[i+1]-jd_dates[0]));
        print_date(convert_JD_date(jd_dates[i+1]),0);
        printf(") - |% 5d days | - %s", (int)data[0], bodies[i+1]->name);
        if(i == num_bodies-2) printf(" (%.2f m/s)\nTotal: %.2f m/s\n", data[2], dep_dv+data[2]);
        else printf("\n");

        struct OSV osvs[3];
        osvs[0].r = transfer.r0;
        osvs[0].v = transfer.v0;
        osvs[1].r = transfer.r1;
        osvs[1].v = transfer.v1;
        osvs[2] = s1;

        for (int j = 0; j < 3; j++) {
            transfer_data[(int)transfer_data[0] + 1] = j == 0 ? jd_dates[i] : jd_dates[i+1];
            transfer_data[(int)transfer_data[0] + 2] = osvs[j].r.x;
            transfer_data[(int)transfer_data[0] + 3] = osvs[j].r.y;
            transfer_data[(int)transfer_data[0] + 4] = osvs[j].r.z;
            transfer_data[(int)transfer_data[0] + 5] = osvs[j].v.x;
            transfer_data[(int)transfer_data[0] + 6] = osvs[j].v.y;
            transfer_data[(int)transfer_data[0] + 7] = osvs[j].v.z;
            transfer_data[0] += 7;
        }
    }

    for(int i = 0; i < num_bodies; i++) {
        int ephem_double = 0;
        for(int j = 0; j < i; j++) {
            if(bodies[i] == bodies[j]) {
                ephems[i] = ephems[j];
                ephem_double = 1;
                break;
            }
        }
        if(ephem_double) continue;
        free(ephems[i]);
    }
    free(ephems);
    free(jd_dates);

    char pcsv;
    printf("Write transfer data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if (pcsv == 'y' || pcsv == 'Y') {
        char transfer_data_fields[] = "JD,X,Y,Z,VX,VY,VZ";
        write_csv(transfer_data_fields, transfer_data);
    }
}
