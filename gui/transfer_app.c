#include "transfer_app.h"
#include "analytic_geometry.h"
#include "ephem.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "drawing.h"
#include "orbit_calculator/orbit.h"

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>


struct TransferData {
	struct Body *body;
	double date;
	double dv;
	struct TransferData *prev;
	struct TransferData *next;
};


struct TransferData *curr_transfer;

static int counter = 0;
GObject *drawing_area;
GObject *lb_date;
GObject *tb_tfdate;
GObject *bt_tfbody;
GObject *lb_transfer_dv;
GObject *lb_total_dv;
GObject *transfer_panel;

gboolean body_show_status[9];
struct Ephem **ephems;

double current_date;

void update_date_label();
void update_transfer_panel();
void activate(GtkApplication *app, gpointer user_data);
void on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_body_toggle(GtkWidget* widget, gpointer data);
void on_change_date(GtkWidget* widget, gpointer data);
void on_calendar_selection(GtkWidget* widget, gpointer data);
void on_prev_transfer(GtkWidget* widget, gpointer data);
void on_next_transfer(GtkWidget* widget, gpointer data);
void on_transfer_body_change(GtkWidget* widget, gpointer data);
void on_toggle_transfer_date_lock(GtkWidget* widget, gpointer data);
void on_goto_transfer_date(GtkWidget* widget, gpointer data);
void on_transfer_body_select(GtkWidget* widget, gpointer data);
void on_add_transfer(GtkWidget* widget, gpointer data);
void on_remove_transfer(GtkWidget* widget, gpointer data);





void start_transfer_app() {
	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	
	ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}
	struct Date date = {2000, 1, 1, 0, 0, 0};
	current_date = convert_date_JD(date);

#ifdef GTK_SRCDIR
	g_chdir (GTK_SRCDIR);
#endif
	
	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	
	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
	free(ephems);
}

void activate(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/transfer.glade", NULL);
	
	gtk_builder_connect_signals(builder, NULL);
	
	tb_tfdate = gtk_builder_get_object(builder, "tb_tfdate");
	bt_tfbody = gtk_builder_get_object(builder, "bt_tfbody");
	lb_transfer_dv = gtk_builder_get_object(builder, "lb_transfer_dv");
	lb_total_dv = gtk_builder_get_object(builder, "lb_total_dv");
	transfer_panel = gtk_builder_get_object(builder, "transfer_panel");
	lb_date = gtk_builder_get_object(builder, "lb_date");
	update_date_label();
	update_transfer_panel();
	
	
	drawing_area = gtk_builder_get_object(builder, "drawing_area");
	g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
	
	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);
	
	/* We do not need the builder anymore */
	g_object_unref(builder);
}





void on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int window_width = allocation.width;
	int window_height = allocation.height;
	struct Vector2D center = {(double) window_width/2, (double) window_height/2};
	
	// reset drawing area
	cairo_rectangle(cr, 0, 0, window_width, window_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);
	
	// Scale
	int highest_id = 0;
	for(int i = 0; i < 9; i++) if(body_show_status[i]) highest_id = i+1;
	double scale = calc_scale(window_width, window_height, highest_id);
	
	// Sun
	set_cairo_body_color(cr, 0);
	draw_body(cr, center, 0, vec(0,0,0));
	cairo_fill(cr);
	
	// Planets
	for(int i = 0; i < 9; i++) {
		if(body_show_status[i]) {
			int id = i+1;
			set_cairo_body_color(cr, id);
			struct OSV osv = osv_from_ephem(ephems[i], current_date, SUN());
			draw_body(cr, center, scale, osv.r);
			draw_orbit(cr, center, scale, osv.r, osv.v, SUN());
		}
	}
}

void update_date_label() {
	char date_string[10];
	date_to_string(convert_JD_date(current_date), date_string, 0);
	gtk_label_set_text(GTK_LABEL(lb_date), date_string);
}

void update_transfer_panel() {
	if(curr_transfer == NULL) {
		gtk_button_set_label(GTK_BUTTON(tb_tfdate), "0000-00-00");
	} else {
		struct Date date = convert_JD_date(curr_transfer->date);
		char date_string[10];
		date_to_string(date, date_string, 0);
		gtk_button_set_label(GTK_BUTTON(tb_tfdate), date_string);
	}
}


void on_body_toggle(GtkWidget* widget, gpointer data) {
	int id = (int) gtk_widget_get_name(widget)[0] - 48;
	body_show_status[id-1] = body_show_status[id-1] ? 0 : 1;
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}


void on_change_date(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "+1Y") == 0) current_date = jd_change_date(current_date, 1, 0, 0);
	else if	(strcmp(name, "+1M") == 0) current_date = jd_change_date(current_date, 0, 1, 0);
	else if	(strcmp(name, "+1D") == 0) current_date++;
	else if	(strcmp(name, "-1Y") == 0) current_date = jd_change_date(current_date,-1, 0, 0);
	else if	(strcmp(name, "-1M") == 0) current_date = jd_change_date(current_date, 0,-1, 0);
	else if	(strcmp(name, "-1D") == 0) current_date--;
	update_date_label();
	if(curr_transfer != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tfdate))) {
		if((curr_transfer->next != NULL && current_date >= curr_transfer->next->date) ||
		   (curr_transfer->prev != NULL && current_date <= curr_transfer->prev->date)) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
			return;
		}
		curr_transfer->date = current_date;
		update_transfer_panel();
	}
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}


void on_year_select(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	int year = atoi(name);
	struct Date date = {year, 1,1};
	current_date = convert_date_JD(date);
	update_date_label();
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}



void on_prev_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL) return;
	if(curr_transfer->prev != NULL) curr_transfer = curr_transfer->prev;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
	update_transfer_panel();
}

void on_next_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL) return;
	if(curr_transfer->next != NULL) curr_transfer = curr_transfer->next;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
	update_transfer_panel();
}

void on_transfer_body_change(GtkWidget* widget, gpointer data) {
	gtk_stack_set_visible_child_name(GTK_STACK(transfer_panel), "page1");
}

void on_toggle_transfer_date_lock(GtkWidget* widget, gpointer data) {
	if(curr_transfer != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tfdate))) {
		if((curr_transfer->next != NULL && current_date >= curr_transfer->next->date) ||
			(curr_transfer->prev != NULL && current_date <= curr_transfer->prev->date)) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
			return;
		}
		curr_transfer->date = current_date;
		update_transfer_panel();
		gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
	}
}

void on_goto_transfer_date(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL) return;
	current_date = curr_transfer->date;
	update_date_label();
}

void on_transfer_body_select(GtkWidget* widget, gpointer data) {
	gtk_stack_set_visible_child_name(GTK_STACK(transfer_panel), "page0");
}

void on_add_transfer(GtkWidget* widget, gpointer data) {
	struct TransferData *new_transfer = (struct TransferData *) malloc(sizeof(struct TransferData));
	new_transfer->body = EARTH();
	new_transfer->dv   = 0;
	new_transfer->prev = curr_transfer;
	if(curr_transfer != NULL) {
		new_transfer->next = curr_transfer->next;
		curr_transfer->next = new_transfer;
		new_transfer->date = current_date > curr_transfer->date ? current_date : curr_transfer->date+1;
	} else {
		new_transfer->next = NULL;
		new_transfer->date = current_date;
	}
	curr_transfer = new_transfer;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
	update_transfer_panel();
}

void on_remove_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL) return;
	struct TransferData *rem_transfer = curr_transfer;
	if(curr_transfer->next != NULL) curr_transfer->next->prev = curr_transfer->prev;
	if(curr_transfer->prev != NULL) curr_transfer->prev->next = curr_transfer->next;
	curr_transfer = curr_transfer->prev != NULL ? curr_transfer->prev : curr_transfer->next;
	free(rem_transfer);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
	update_transfer_panel();
}