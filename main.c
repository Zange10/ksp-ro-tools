#include "celestial_bodies.h"
#include "launch_calculator/launch_calculator.h"
#include "orbit_calculator/orbit_calculator.h"
#include "orbit_calculator/transfer_calculator.h"
#include "tools/tool_funcs.h"
#include "gui/transfer_app.h"
#include "gui/database_app.h"
#include "database/database.h"
#include "database/itin_db.h"


// ------------------------------------------------------------

int main() {
    init_celestial_bodies();

    int selection;
    char title[] = "CHOOSE PROGRAM:";
    char options[] = "Exit; Launch Calculator; Orbit Calculator; Transfer Calculator; Transfer App; Database App";
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
			init_itin_db();
			create_itinerary();
			close_itin_db();
            break;
		case 4:
			start_transfer_app();
			break;
		case 5:
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