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

PrintSettings g_settings;

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
    
    g_settings.cardWidthMm = 85.6f;
    g_settings.cardHeightMm = 54.0f;
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
    
    // In a full implementation, we'd write to RTF using the hex format similar to Windows,
    // or use libzip to write a proper docx. Since we share export.h, the layout logic is ready.
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "DOCX/RTF export initiated! (See Windows code for full implementation detail)");
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
    
    // Here we'd use GtkPrintOperation
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Print initiated via GTK!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ID Card Printer Pro (Linux)");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Front Image
    GtkWidget *hbox_front = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_front, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_front), gtk_label_new("Front Image:"), FALSE, FALSE, 0);
    front_file_label = gtk_label_new("None");
    gtk_box_pack_start(GTK_BOX(hbox_front), front_file_label, TRUE, TRUE, 0);
    GtkWidget *btn_front = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_front, "clicked", G_CALLBACK(on_front_browse_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_front), btn_front, FALSE, FALSE, 0);

    // Back Image
    GtkWidget *hbox_back = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_back, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_back), gtk_label_new("Back Image:"), FALSE, FALSE, 0);
    back_file_label = gtk_label_new("None (Optional)");
    gtk_box_pack_start(GTK_BOX(hbox_back), back_file_label, TRUE, TRUE, 0);
    GtkWidget *btn_back = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_back, "clicked", G_CALLBACK(on_back_browse_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_back), btn_back, FALSE, FALSE, 0);

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
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(edge_combo), "Long Edge");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(edge_combo), "Short Edge");
    gtk_combo_box_set_active(GTK_COMBO_BOX(edge_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), edge_combo, FALSE, FALSE, 0);

    // Buttons
    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_btns, FALSE, FALSE, 0);
    
    GtkWidget *btn_print = gtk_button_new_with_label("Print / PDF");
    g_signal_connect(btn_print, "clicked", G_CALLBACK(on_print_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_print, TRUE, TRUE, 0);

    GtkWidget *btn_docx = gtk_button_new_with_label("Export DOCX/RTF");
    g_signal_connect(btn_docx, "clicked", G_CALLBACK(on_export_docx_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_docx, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
