#include <stdio.h>

#include "launch_calculator.h"
#include "orbit_calculator.h"
#include "tool_funcs.h"

// ------------------------------------------------------------

int main() {
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