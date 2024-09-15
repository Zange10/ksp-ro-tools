#include "mission_db.h"
#include "database/mission_database.h"
#include "database/lv_database.h"
#include "gui/css_loader.h"


GObject *mission_vp;
GObject *tf_mdbfilt_name;
GObject *cb_mdbfilt_program;
GObject *cb_mdbfilt_ytf;
GObject *cb_mdbfilt_inprog;
GObject *cb_mdbfilt_ended;
GObject *bt_mdb_reset;
GObject *bt_mdb_show;
GtkWidget *mission_grid;


void update_db_box();
void update_program_dropdown();


void init_mission_db(GtkBuilder *builder) {
	mission_vp = gtk_builder_get_object(builder, "vp_missiondb");
	tf_mdbfilt_name = gtk_builder_get_object(builder, "tf_mdbfilt_name");
	cb_mdbfilt_program = gtk_builder_get_object(builder, "cb_mdbfilt_program");
	cb_mdbfilt_ytf = gtk_builder_get_object(builder, "cb_mdbfilt_ytf");
	cb_mdbfilt_inprog = gtk_builder_get_object(builder, "cb_mdbfilt_inprog");
	cb_mdbfilt_ended = gtk_builder_get_object(builder, "cb_mdbfilt_ended");
	bt_mdb_reset = gtk_builder_get_object(builder, "bt_mdb_reset");
	bt_mdb_show = gtk_builder_get_object(builder, "bt_mdb_show");

	// Create a cell renderer for dropdowns/ComboBox
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_mdbfilt_program), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_mdbfilt_program), renderer, "text", 0, NULL);

	update_program_dropdown();
	update_db_box();
}




void update_db_box() {
	// Remove grid if exists
	if (mission_grid != NULL && GTK_WIDGET(mission_vp) == gtk_widget_get_parent(mission_grid)) {
		gtk_container_remove(GTK_CONTAINER(mission_vp), mission_grid);
	}

	mission_grid = gtk_grid_new();
	GtkWidget *separator;

	struct Mission_DB *missions;
	struct MissionProgram_DB *programs;
	int num_programs = db_get_all_programs(&programs);

	struct Mission_Filter filter;
	sprintf(filter.name, "%s", (char*) gtk_entry_get_text(GTK_ENTRY(tf_mdbfilt_name)));
	filter.program_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_mdbfilt_program));
	filter.ytf 		= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_ytf));
	filter.in_prog 	= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_inprog));
	filter.ended	= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_ended));
	int mission_count = db_get_missions_ordered_by_launch_date(&missions, filter);


	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, 0, MISSION_COLS*2+1, 1);
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, 1, 1, 1);

	for (int col = 0; col < MISSION_COLS; ++col) {
		char label_text[20];
		int req_width = -1;
		switch(col) {
			case 1: sprintf(label_text, "Mission"); req_width = 150; break;
			case 2: sprintf(label_text, "Program"); req_width = 120; break;
			case 3: sprintf(label_text, "Status"); req_width = 120; break;
			case 4: sprintf(label_text, "Vehicle"); req_width = 150; break;
			default:sprintf(label_text, "#"); req_width = 20; break;
		}

		// Create a GtkLabel
		GtkWidget *label = gtk_label_new(label_text);
		// width request
		gtk_widget_set_size_request(GTK_WIDGET(label), req_width, -1);
		// set css class
		set_css_class_for_widget(GTK_WIDGET(label), "missiondb-header");

		// Set the label in the grid at the specified row and column
		gtk_grid_attach(GTK_GRID(mission_grid), label, col*2+1, 1, 1, 1);

		// Create a horizontal separator line (optional)
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(mission_grid), separator, col*2+2, 1, 1, 1);
	}

	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, 2, MISSION_COLS*2+1, 1);

	struct LauncherInfo_DB lv;
	struct PlaneInfo_DB fv;

	// Create labels and add them to the grid
	for (int i = 0; i < mission_count; ++i) {
		struct Mission_DB m = missions[i];
		int row = i*2+3;

		for (int j = 0; j < MISSION_COLS; ++j) {
			int col = j*2+1;
			char label_text[30];
			if(m.launcher_id != 0) lv = db_get_launcherInfo_from_id(m.launcher_id);
			else if(m.plane_id != 0) fv = db_get_plane_from_id(m.plane_id);
			switch(j) {
				case 1: sprintf(label_text, "%s", m.name); break;
				case 2: sprintf(label_text, "%s", programs[m.program_id-1].name); break;
				case 3: sprintf(label_text, "%s",(m.status == YET_TO_FLY ? "YET TO FLY" : (m.status == IN_PROGRESS ? "IN PROGRESS" : "ENDED"))); break;
				case 4:
					if(m.launcher_id != 0) sprintf(label_text, "%s", lv.name);
					else if(m.plane_id != 0) sprintf(label_text, "%s", fv.name);
					else sprintf(label_text, "/"); break;
				default:sprintf(label_text, "%d", i+1); break;
			}

			// Create a GtkLabel
			GtkWidget *label = gtk_label_new(label_text);
			// left-align (and right-align for id)
			gtk_label_set_xalign(GTK_LABEL(label), (gfloat) 0.0);
			if(j == 0) gtk_label_set_xalign(GTK_LABEL(label), (gfloat) 1.0);
			// set css class
			set_css_class_for_widget(GTK_WIDGET(label), "missiondb-hardpartial-mission");

			// Set the label in the grid at the specified row and column
			gtk_grid_attach(GTK_GRID(mission_grid), label, col, row, 1, 1);

			// Create a horizontal separator line (optional)
			separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
			gtk_grid_attach(GTK_GRID(mission_grid), separator, col+1, row, 1, 1);
		}
		separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(mission_grid), separator, 0, row+1, MISSION_COLS*2+1, 1);
	}

	gtk_container_add (GTK_CONTAINER (mission_vp),
					   mission_grid);
	gtk_widget_show_all(GTK_WIDGET(mission_vp));

	free(missions);
	free(programs);
}

void update_program_dropdown() {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;

	struct MissionProgram_DB *programs;
	int num_programs = db_get_all_programs(&programs);


	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "ALL", 1, 0, -1);
	// Add items to the list store
	char entry[50];
	for(int i = 0; i < num_programs; i++) {
		gtk_list_store_append(store, &iter);
		sprintf(entry, "%s", programs[i].name);
		gtk_list_store_set(store, &iter, 0, entry, 1, i+1, -1);
	}

	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_mdbfilt_program), GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_mdbfilt_program), 0);

	g_object_unref(store);
	free(programs);
}

void on_show_missions(GtkWidget* widget, gpointer data) {
	update_db_box();
}

void on_reset_mission_filter(GtkWidget* widget, gpointer data) {
	gtk_entry_set_text(GTK_ENTRY(tf_mdbfilt_name), "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_ended), 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_inprog), 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_mdbfilt_ytf), 1);
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_mdbfilt_program), 0);
	update_db_box();
}

void close_mission_db() {

}
