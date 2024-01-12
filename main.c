#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "tool_funcs.h"
#include "ephem.h"

// ------------------------------------------------------------

int main() {
	for(int i = 3; i <= 9; i++) {
		// Create a directory
		char directoryPath[50];
		sprintf(directoryPath, "Ephems/%d", i);
		mkdir(directoryPath, 0777);
		
		printf("Directory \"%s\" created successfully.\n", directoryPath);
		
		for(int j = 0; j < 10; j++) {
			struct Date begin_date = {1950+j*10, 1,1,0,0};
			struct Date end_date = {1950+j*10+9, 12,31,0,0};
			double jd0 = convert_date_JD(begin_date);
			double jd1 = convert_date_JD(end_date);
			// Create a file in the directory
			char file_path[50];
			sprintf(file_path, "Ephems/%d/%d.ephem", i, 1950 + j*10);
			
			print_ephem_to_file(file_path, i, jd0, jd1);
		}
	}
    return 0;
    init_celestial_bodies();

    int selection = 0;
    char title[] = "CHOOSE PROGRAM:";
    char options[] = "Exit; Launch Calculator; Orbit Calculator";
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
        default:
            break;
        }
    } while(selection != 0);
    return 0;
}