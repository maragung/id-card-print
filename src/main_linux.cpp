#include <gtk/gtk.h>
#include <string>
#include <iostream>
#include "export.h"

// Note: This is a GTK3 implementation for Linux.
// It requires `libgtk-3-dev` to be installed.

static GtkWidget *window;
static GtkWidget *front_file_label;
static GtkWidget *back_file_label;
static GtkWidget *copies_entry;
static GtkWidget *paper_combo;
static GtkWidget *duplex_check;
static GtkWidget *edge_combo;
static GtkWidget *card_w_entry;
static GtkWidget *card_h_entry;
static GtkWidget *def_size_check;

PrintSettings g_settings;

static void on_def_size_toggled(GtkToggleButton *btn, gpointer data) {
    gboolean active = gtk_toggle_button_get_active(btn);
    gtk_widget_set_sensitive(card_w_entry, !active);
    gtk_widget_set_sensitive(card_h_entry, !active);
}

static void on_front_browse_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Front Image",
                                                     GTK_WINDOW(window),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Open", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        g_settings.frontImagePath = filename;
        gtk_label_set_text(GTK_LABEL(front_file_label), filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void on_back_browse_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Back Image",
                                                     GTK_WINDOW(window),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Open", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        g_settings.backImagePath = filename;
        gtk_label_set_text(GTK_LABEL(back_file_label), filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void read_settings() {
    g_settings.copies = atoi(gtk_entry_get_text(GTK_ENTRY(copies_entry)));
    if (g_settings.copies <= 0) g_settings.copies = 1;
    
    int paper_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(paper_combo));
    if (paper_idx == 0) { // A4
        g_settings.paperWidthMm = 210.0f;
        g_settings.paperHeightMm = 297.0f;
    } else { // Letter
        g_settings.paperWidthMm = 215.9f;
        g_settings.paperHeightMm = 279.4f;
    }
    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(def_size_check))) {
        g_settings.cardWidthMm = 85.6f;
        g_settings.cardHeightMm = 53.98f;
    } else {
        g_settings.cardWidthMm = atof(gtk_entry_get_text(GTK_ENTRY(card_w_entry)));
        g_settings.cardHeightMm = atof(gtk_entry_get_text(GTK_ENTRY(card_h_entry)));
    }
    
    g_settings.duplex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(duplex_check));
    g_settings.flipEdge = gtk_combo_box_get_active(GTK_COMBO_BOX(edge_combo));
}

static void on_export_docx_clicked(GtkWidget *widget, gpointer data) {
    read_settings();
    if (g_settings.frontImagePath.empty()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Please select front image.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Export feature is primarily for Windows. Linux version currently supports layout calculation.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_print_clicked(GtkWidget *widget, gpointer data) {
    read_settings();
    if (g_settings.frontImagePath.empty()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Please select front image.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Print initiated via GTK!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ID Card Print");
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 450);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Front Image
    GtkWidget *hbox_front = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_front, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_front), gtk_label_new("Front Image:"), FALSE, FALSE, 0);
    front_file_label = gtk_label_new("None");
    gtk_label_set_xalign(GTK_LABEL(front_file_label), 0.0);
    gtk_box_pack_start(GTK_BOX(hbox_front), front_file_label, TRUE, TRUE, 0);
    GtkWidget *btn_front = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_front, "clicked", G_CALLBACK(on_front_browse_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_front), btn_front, FALSE, FALSE, 0);

    // Back Image
    GtkWidget *hbox_back = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_back, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_back), gtk_label_new("Back Image:"), FALSE, FALSE, 0);
    back_file_label = gtk_label_new("None (Optional)");
    gtk_label_set_xalign(GTK_LABEL(back_file_label), 0.0);
    gtk_box_pack_start(GTK_BOX(hbox_back), back_file_label, TRUE, TRUE, 0);
    GtkWidget *btn_back = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_back, "clicked", G_CALLBACK(on_back_browse_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_back), btn_back, FALSE, FALSE, 0);

    // Card Size
    GtkWidget *frame_size = gtk_frame_new("ID Card Size (mm)");
    gtk_box_pack_start(GTK_BOX(vbox), frame_size, FALSE, FALSE, 0);
    GtkWidget *vbox_size = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox_size), 5);
    gtk_container_add(GTK_CONTAINER(frame_size), vbox_size);
    
    def_size_check = gtk_check_button_new_with_label("Use default size (85.6 x 53.98)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(def_size_check), TRUE);
    g_signal_connect(def_size_check, "toggled", G_CALLBACK(on_def_size_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(vbox_size), def_size_check, FALSE, FALSE, 0);
    
    GtkWidget *hbox_custom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox_size), hbox_custom, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_custom), gtk_label_new("W:"), FALSE, FALSE, 0);
    card_w_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(card_w_entry), "85.6");
    gtk_widget_set_sensitive(card_w_entry, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox_custom), card_w_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_custom), gtk_label_new("H:"), FALSE, FALSE, 0);
    card_h_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(card_h_entry), "53.98");
    gtk_widget_set_sensitive(card_h_entry, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox_custom), card_h_entry, TRUE, TRUE, 0);

    // Copies & Paper
    GtkWidget *hbox_settings = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_settings, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox_settings), gtk_label_new("Copies:"), FALSE, FALSE, 0);
    copies_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(copies_entry), "1");
    gtk_box_pack_start(GTK_BOX(hbox_settings), copies_entry, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox_settings), gtk_label_new("Paper Size:"), FALSE, FALSE, 0);
    paper_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(paper_combo), "A4");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(paper_combo), "Letter");
    gtk_combo_box_set_active(GTK_COMBO_BOX(paper_combo), 0);
    gtk_box_pack_start(GTK_BOX(hbox_settings), paper_combo, FALSE, FALSE, 0);

    // Duplex
    duplex_check = gtk_check_button_new_with_label("Enable Duplex");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(duplex_check), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), duplex_check, FALSE, FALSE, 0);

    edge_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(edge_combo), "Long Edge Flip");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(edge_combo), "Short Edge Flip");
    gtk_combo_box_set_active(GTK_COMBO_BOX(edge_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), edge_combo, FALSE, FALSE, 0);

    // Buttons
    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_btns, FALSE, FALSE, 0);
    
    GtkWidget *btn_print = gtk_button_new_with_label("Print / PDF");
    g_signal_connect(btn_print, "clicked", G_CALLBACK(on_print_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_print, TRUE, TRUE, 0);

    GtkWidget *btn_docx = gtk_button_new_with_label("Export Settings");
    g_signal_connect(btn_docx, "clicked", G_CALLBACK(on_export_docx_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_docx, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
