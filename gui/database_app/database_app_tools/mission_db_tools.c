#include "mission_db_tools.h"
#include "gui/css_loader.h"


int get_highest_combobox_iter_id(GtkComboBox *combobox);

// Function to get the active program ID from the combo box
int get_active_combobox_id(GtkComboBox *combo_box) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	int program_id = -1;  // Default value if no program is selected

	// Get the active row's iterator
	if (gtk_combo_box_get_active_iter(combo_box, &iter)) {
		// Get the model from the combo box
		model = gtk_combo_box_get_model(combo_box);

		// Retrieve the 'id' from column 1 (assuming id is stored in the second column)
		gtk_tree_model_get(model, &iter, 1, &program_id, -1);
	}

	return program_id;
}

// Function to set the active row based on a specific ID
void set_active_combobox_id(GtkComboBox *combobox, int target_id) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	int id;
	if(target_id < 0) target_id = get_highest_combobox_iter_id(combobox);
	// Get the model from the combobox
	model = gtk_combo_box_get_model(combobox);

	// Start iterating through the model to find the matching ID
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			// Get the value of the ID in the specified column
			gtk_tree_model_get(model, &iter, 1, &id, -1);

			// If the current row's ID matches the target ID, set it as active
			if (id == target_id) {
				gtk_combo_box_set_active_iter(combobox, &iter);
				return;  // Exit once the correct ID is found and set
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}

	g_print("ID %d not found in combobox.\n", target_id);
}

struct MissionProgram_DB get_program_from_id(struct MissionProgram_DB *programs, int num_programs, int program_id) {
	for(int i = 0; i < num_programs; i++) if(programs[i].id == program_id) return programs[i];
	return programs[0];
}

struct Mission_DB get_mission_from_id(struct Mission_DB *missions, int num_missions, int mission_id) {
	for(int i = 0; i < num_missions; i++) if(missions[i].id == mission_id) return missions[i];
	return missions[0];
}


void set_gtk_widget_class_by_mission_success(GtkWidget *widget, struct Mission_DB mission) {
	if(mission.status == ENDED) {
		enum MissionSuccess mission_success = db_get_mission_success(mission.id);
		if(mission_success == MISSION_SUCCESS)
			set_css_class_for_widget(widget, "missiondb-successful-mission");
		else if(mission_success == MISSION_PARTIAL_SUCCESS)
			set_css_class_for_widget(widget, "missiondb-partial-mission");
		else if(mission_success == MISSION_FAIL)
			set_css_class_for_widget(widget, "missiondb-failure-mission");
		else
			set_css_class_for_widget(widget, "missiondb-inprog-mission");
	} else if(mission.status == IN_PROGRESS) {
		set_css_class_for_widget(widget, "missiondb-inprog-mission");
	} else {
		set_css_class_for_widget(widget, "missiondb-ytf-mission");
	}
}

// Function to find the highest ID in a combobox model
int get_highest_combobox_iter_id(GtkComboBox *combobox) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	int highest_id = -1;  // Assume IDs are positive, so initialize to a low value

	// Get the model from the combobox
	model = gtk_combo_box_get_model(combobox);

	// Start iterating through the model
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			int id;
			// Get the value of the ID in the specified column
			gtk_tree_model_get(model, &iter, 1, &id, -1);

			// Update highest_id if the current id is larger
			if (id > highest_id) {
				highest_id = id;
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}

	return highest_id;
}
