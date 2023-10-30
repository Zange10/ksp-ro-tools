#include "ephem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_ephem(struct Ephem ephem) {
    printf("\nDate: %f\nx: %g m,   y: %g m,   z: %g m\n"
           "vx: %g m/s,   vy: %g m/s,   vz: %g m/s\n\n",
           ephem.date, ephem.x, ephem.y, ephem.z, ephem.vx, ephem.vy, ephem.vz);
}

void print_date(struct Date date, int line_break) {
    printf("%4d-%02d-%02d %02d:%02d:%06.3f", date.y, date.m, date.d, date.h, date.min, date.s);
    if(line_break) printf("\n");
}

struct Date convert_JD_date(double JD) {
    struct Date date = {0,1,1,0,0,0};
//    printf("%f\n", JD);
    double init_JD = JD;
    JD -= 2451544.5;
    date.y = JD >= 0 ?  2000 + (int)(JD/365.25) : 2000 + (int)(JD/365.25)-1;
    JD = init_JD - convert_date_JD(date);
//    printf("%f\n", JD);

    for(int i = 1; i < 12; i++) {
        int month_days;
        if(i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10) month_days = 31;
        else if(i == 4 || i == 6 || i == 9 || i == 11) month_days = 30;
        else {
            if(date.y%4 == 0) month_days = 29;
            else month_days = 28;
        }
        JD -= month_days;
        if(JD < 0) {
            JD += month_days;
            break;
        } else {
            date.m++;
        }
    }

//    printf("%f\n", JD);
    date.d += (int)JD;
    JD -= (int)JD;

    // years before a leap year before 2000 would go to 32 Dec
    if(date.m == 12 && date.d == 32) {
        date.m = 1;
        date.d = 1;
        date.y++;
    }

//    printf("%f\n", JD);

    date.h = (int) (JD * 24.0);
    JD -= (double)date.h/24;
    date.min = (int) (JD * 24 * 60);
    JD -= (double)date.min/(24*60);
    date.s = JD;

    return date;
}

double convert_date_JD(struct Date date) {
    double J = 2451544.5;     // 2000-01-01 00:00
    int diff_year = date.y-2000;
    int year_part = diff_year * 365.25;
    if(date.y < 2000 || date.y%4 == 0) J -= 1; // leap years do leap day themselves after 2000

    int month_part = 0;
    for(int i = 1; i < 12; i++) {
        if(i==date.m) break;
        if(i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10) month_part += 31;
        else if(i == 4 || i == 6 || i == 9 || i == 11) month_part += 30;
        else {
            if(date.y%4 == 0) month_part += 29;
            else month_part += 28;
        }
    }

    J += month_part+year_part+date.d;
    J += (double)date.h/24 + (double)date.min/(24*60) + date.s/(24*60*60);
    return J;
}

void get_ephem(struct Ephem *ephem, double size_ephem) {
    // Construct the URL with your API key and parameters
    const char *url = "https://ssd.jpl.nasa.gov/api/horizons.api?"
                      "format=text&"
                      "COMMAND='2'&"
                      "OBJ_DATA='NO'&"
                      "MAKE_EPHEM='YES'&"
                      "EPHEM_TYPE='VECTORS'&"
                      "CENTER='500@0'&"
                      "START_TIME='2000-01-01'&"
                      "STOP_TIME='2005-01-01'&"
                      "STEP_SIZE='10d'&"
                      "VEC_TABLE='2'&"
                      "QUANTITIES='1,9,20,23,24,29'";

    // Construct the wget command
    char wget_command[512];
    snprintf(wget_command, sizeof(wget_command), "wget \"%s\" -O output.json", url);

    // Execute the wget command
    int ret_code = system(wget_command);

    // Check for errors
    if (ret_code != 0) {
        fprintf(stderr, "Error executing wget: %d\n", ret_code);
        return;
    }
    // Now you can parse the downloaded JSON file (output.json) in your C program.

    FILE *file;
    char line[256];  // Assuming lines are no longer than 255 characters

    // Open the file for reading
    file = fopen("output.json", "r");

    if (file == NULL) {
        perror("Unable to open file");
        return;
    }

    // Read lines from the file until the end is reached
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "$$SOE") == 0) {
            break; // Exit the loop when "$$SOE" is encountered
        }
    }

    for(int i = 0; i < size_ephem; i++){
        fgets(line, sizeof(line), file);
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "$$EOE") == 0) {
            break; // Exit the loop when "$$SOE" is encountered
        }
        char *endptr;
        double date = strtod(line, &endptr);

        fgets(line, sizeof(line), file);
        double x,y,z;
        sscanf(line, " X =%lf Y =%lf Z =%lf", &x, &y, &z);
        fgets(line, sizeof(line), file);
        double vx,vy,vz;
        sscanf(line, " VX=%lf VY=%lf VZ=%lf", &vx, &vy, &vz);

        ephem[i].date = date;
        ephem[i].x = x*1e3;
        ephem[i].y = y*1e3;
        ephem[i].z = z*1e3;
        ephem[i].vx = vx*1e3;
        ephem[i].vy = vy*1e3;
        ephem[i].vz = vz*1e3;
    }

    // Close the file when done
    fclose(file);
    return;
}
