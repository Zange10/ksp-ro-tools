#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/transfer_app.h"
#include "gui/database_app.h"
#include "database/database.h"
#include "orbit_calculator/transfer_calc.h"
#include <stdlib.h>


// ------------------------------------------------------------

int main() {
    init_celestial_bodies();

	struct Date min_date = {1997, 10, 1};	// Cassini: 1997-10-06
	struct Date max_date = {1997, 10, 10};


	int num_steps = 5;
	struct Transfer_Calc_Data calc_data;
	calc_data.num_steps = num_steps;
	calc_data.jd_min_dep = convert_date_JD(min_date);
	calc_data.jd_max_dep = convert_date_JD(max_date);
	calc_data.dv_filter.max_totdv = 1e9;
	calc_data.dv_filter.max_depdv = 1e9;
	calc_data.dv_filter.max_satdv = 1e9;
	calc_data.dv_filter.last_transfer_type = 0;

	calc_data.bodies = (struct Body**) malloc(num_steps * sizeof(struct Body*));
	calc_data.min_duration = (int*) malloc((num_steps-1) * sizeof(int));
	calc_data.max_duration = (int*) malloc((num_steps-1) * sizeof(int));

	int counter = 0;
	calc_data.bodies[counter] = EARTH();
	counter++;

	calc_data.bodies[counter] = VENUS();
	calc_data.min_duration[counter-1] = 197;	// Cassini 197
	calc_data.max_duration[counter-1] = 197;
	counter++;

	calc_data.bodies[counter] = VENUS();
	calc_data.min_duration[counter-1] = 423;	// Cassini 425
	calc_data.max_duration[counter-1] = 427;
	counter++;

	calc_data.bodies[counter] = EARTH();
	calc_data.min_duration[counter-1] = 20;	// Cassini 65
	calc_data.max_duration[counter-1] = 90;
	counter++;

	calc_data.bodies[counter] = JUPITER();
	calc_data.min_duration[counter-1] = 200;	// Cassini 559
	calc_data.max_duration[counter-1] = 2000;
	counter++;

//	calc_data.bodies[counter] = SATURN();
//	calc_data.min_duration[counter-1] = 800;	// Cassini 1273
//	calc_data.max_duration[counter-1] = 2000;


	struct Transfer_Calc_Results results = create_itinerary(calc_data);


	struct ItinStep **all_arrivals, *temp;


	int num_itins = 0, tot_num_itins = 0, num_deps = results.num_deps;
	for(int i = 0; i < num_deps; i++) num_itins += get_number_of_itineraries(results.departures[i]);
	for(int i = 0; i < num_deps; i++) tot_num_itins += get_total_number_of_stored_steps(results.departures[i]);

	int index = 0;
	all_arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(results.departures[i], all_arrivals, &index);


	double *dv = (double*) malloc(num_itins*sizeof(double));
	double **t = (double**) malloc((num_steps+1)*sizeof(double*));
	for(int i = 0; i < num_steps+1; i++) {
		t[i] = (double*) malloc(num_itins*sizeof(double));
	}



	for(int i = 0; i < num_itins; i++) {
		temp = all_arrivals[i];
		counter = num_steps;
		while(temp != NULL) {
			if(temp->body == NULL) {
				dv[i] = vector_mag(subtract_vectors(temp->next[0]->v_dep, temp->v_arr));
			}

			t[counter][i] = temp->date;
			counter--;
			temp = temp->prev;
		}
	}


//	for(int i = 0; i < num_itins; i++) {
//		printf("% 6.0f    |    ", dv[i]);
//		for(int j = 0; j < num_steps+1; j++) {
//			if(j != 0) printf(" | ");
//			print_date(convert_JD_date(t[j][i]), 0);
//		}
//		printf("\n");
//	}

	printf("dv = [");
	for(int i = 0; i < num_itins; i++) {
		if(i != 0) printf(",");
		printf("%.2f", dv[i]);
	}
	printf("]\n");

	for(int c = 0; c < num_steps+1; c++) {
		printf("t%d = [", c);
		for(int i = 0; i < num_itins; i++) {
			if(i != 0) printf(",");
			printf("%.2f", t[c][i]);
		}
		printf("]\n");
	}


	free(dv);
	for(int i = 0; i < num_steps+1; i++) free(t[i]);
	free(t);
	return 0;

    int selection;
    char title[] = "CHOOSE PROGRAM:";
    char options[] = "Exit; Launch Calculator; Orbit Calculator; Transfer Calculator; Database App";
    char question[] = "Program: ";

    do {
        selection = user_selection(title, options, question);

        switch (selection) {
        case 1:
            launch_calculator();
            break;
        case 2:
            orbit_calculator();
            break;
		case 3:
			start_transfer_app();
			break;
		case 4:
			init_db();
			start_db_app();
			close_db();
			break;
        default:
            break;
        }
    } while(selection != 0);
    return 0;
}