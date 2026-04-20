#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "export.h"

// GTK3 implementation with Ultra-Compact Sidebar (180px) and High Contrast Themes
// Requires: libgtk-3-dev, pkg-config

struct ImageTransform {
    float rotate = 0.0f;
    float scale = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
};

// Global handles
static GtkWidget *window;
static GtkWidget *front_label, *back_label;
static GtkWidget *paper_combo, *orient_combo, *copies_entry, *duplex_check, *edge_combo;
static GtkWidget *ml_entry, *mr_entry, *mt_entry, *mb_entry;
static GtkWidget *card_w_entry, *card_h_entry, *def_size_check;
static GtkWidget *preview_area;

// State
PrintSettings g_settings;
std::string g_front_path, g_back_path;
ImageTransform g_front_xform, g_back_xform;
int g_preview_page = 0;
int g_current_theme = 0;

// High-Contrast Theme Definitions
const char* THEME_CSS[] = {
    // Light Theme
    "* { color: #000000; font-size: 11px; } "
    "window { background-color: #ffffff; } "
    "frame { border: 1px solid #cccccc; } "
    "entry, combobox, button { background-color: #f5f5f5; border: 1px solid #bbbbbb; }",
    
    // Dark Theme
    "* { color: #ffffff; font-size: 11px; } "
    "window { background-color: #1a1a1a; } "
    "label { color: #ffffff; } "
    "frame { border: 1px solid #444444; color: #ffffff; } "
    "entry, combobox, button { background-color: #333333; color: #ffffff; border: 1px solid #555555; } "
    "entry:focus { border-color: #0078d4; }",
    
    // Azure Theme
    "* { color: #002d5a; font-size: 11px; } "
    "window { background-color: #f0f7ff; } "
    "label { color: #002d5a; } "
    "frame { border: 1px solid #c8dcf0; color: #002d5a; } "
    "entry, combobox, button { background-color: #ffffff; color: #002d5a; border: 1px solid #a0c3e6; }"
};

static void apply_theme() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, THEME_CSS[g_current_theme], -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void on_theme_change(GtkMenuItem *item, gpointer data) {
    g_current_theme = GPOINTER_TO_INT(data);
    apply_theme();
    gtk_widget_queue_draw(preview_area);
}

static void read_ui_settings() {
    g_settings.copies = atoi(gtk_entry_get_text(GTK_ENTRY(copies_entry)));
    if (g_settings.copies <= 0) g_settings.copies = 1;
    g_settings.isLandscape = (gtk_combo_box_get_active(GTK_COMBO_BOX(orient_combo)) == 1);
    int p_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(paper_combo));
    if (p_idx == 0) { g_settings.paperWidthMm = 215.9f; g_settings.paperHeightMm = 279.4f; }
    else { g_settings.paperWidthMm = 210.0f; g_settings.paperHeightMm = 297.0f; }
    g_settings.marginLeft = atof(gtk_entry_get_text(GTK_ENTRY(ml_entry)));
    g_settings.marginRight = atof(gtk_entry_get_text(GTK_ENTRY(mr_entry)));
    g_settings.marginTop = atof(gtk_entry_get_text(GTK_ENTRY(mt_entry)));
    g_settings.marginBottom = atof(gtk_entry_get_text(GTK_ENTRY(mb_entry)));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(def_size_check))) {
        g_settings.cardWidthMm = 85.6f; g_settings.cardHeightMm = 53.98f;
    } else {
        g_settings.cardWidthMm = atof(gtk_entry_get_text(GTK_ENTRY(card_w_entry)));
        g_settings.cardHeightMm = atof(gtk_entry_get_text(GTK_ENTRY(card_h_entry)));
    }
    g_settings.duplex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(duplex_check));
    g_settings.flipEdge = gtk_combo_box_get_active(GTK_COMBO_BOX(edge_combo));
}

static void update_ui_from_settings() {
    char buf[32];
    sprintf(buf, "%d", g_settings.copies); gtk_entry_set_text(GTK_ENTRY(copies_entry), buf);
    gtk_combo_box_set_active(GTK_COMBO_BOX(orient_combo), g_settings.isLandscape ? 1 : 0);
    sprintf(buf, "%.1f", g_settings.marginLeft); gtk_entry_set_text(GTK_ENTRY(ml_entry), buf);
    sprintf(buf, "%.1f", g_settings.marginRight); gtk_entry_set_text(GTK_ENTRY(mr_entry), buf);
    sprintf(buf, "%.1f", g_settings.marginTop); gtk_entry_set_text(GTK_ENTRY(mt_entry), buf);
    sprintf(buf, "%.1f", g_settings.marginBottom); gtk_entry_set_text(GTK_ENTRY(mb_entry), buf);
    sprintf(buf, "%.2f", g_settings.cardWidthMm); gtk_entry_set_text(GTK_ENTRY(card_w_entry), buf);
    sprintf(buf, "%.2f", g_settings.cardHeightMm); gtk_entry_set_text(GTK_ENTRY(card_h_entry), buf);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(duplex_check), g_settings.duplex);
    gtk_combo_box_set_active(GTK_COMBO_BOX(edge_combo), g_settings.flipEdge);
    gtk_label_set_text(GTK_LABEL(front_label), g_front_path.empty() ? "None" : g_front_path.c_str());
    gtk_label_set_text(GTK_LABEL(back_label), g_back_path.empty() ? "None" : g_back_path.c_str());
}

static void draw_card_cairo(cairo_t *cr, const std::string& path, const ImageTransform& xform, float x, float y, float w, float h, float cardW_mm) {
    if (path.empty()) {
        cairo_set_source_rgb(cr, 0.9, 0.9, 0.9); cairo_rectangle(cr, x, y, w, h); cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0.6, 0.6, 0.6); cairo_stroke(cr); return;
    }
    GdkPixbuf *pb = gdk_pixbuf_new_from_file(path.c_str(), NULL); if (!pb) return;
    cairo_save(cr); cairo_rectangle(cr, x, y, w, h); cairo_clip(cr);
    float imgW = gdk_pixbuf_get_width(pb), imgH = gdk_pixbuf_get_height(pb);
    float baseScale = std::min(w / imgW, h / imgH); float pxPerMm = w / cardW_mm;
    cairo_translate(cr, x + w/2.0f + xform.panX * pxPerMm, y + h/2.0f + xform.panY * pxPerMm);
    cairo_rotate(cr, xform.rotate * M_PI / 180.0f); cairo_scale(cr, baseScale * xform.scale, baseScale * xform.scale);
    cairo_translate(cr, -imgW / 2.0f, -imgH / 2.0f); gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);
    cairo_paint(cr); cairo_restore(cr); g_object_unref(pb);
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3); cairo_rectangle(cr, x, y, w, h); cairo_stroke(cr);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    read_ui_settings();
    GtkAllocation alloc; gtk_widget_get_allocation(widget, &alloc);
    if (g_current_theme == 0) cairo_set_source_rgb(cr, 0.98, 0.98, 0.98);
    else if (g_current_theme == 1) cairo_set_source_rgb(cr, 0.125, 0.125, 0.125);
    else cairo_set_source_rgb(cr, 0.92, 0.96, 1.0);
    cairo_paint(cr);
    float pW = g_settings.isLandscape ? g_settings.paperHeightMm : g_settings.paperWidthMm;
    float pH = g_settings.isLandscape ? g_settings.paperWidthMm : g_settings.paperHeightMm;
    float scale = std::min((float)alloc.width / pW, (float)alloc.height / pH) * 0.98f;
    float offX = (alloc.width - pW * scale) / 2.0f, offY = (alloc.height - pH * scale) / 2.0f;
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); cairo_rectangle(cr, offX, offY, pW * scale, pH * scale);
    cairo_fill_preserve(cr); cairo_set_source_rgb(cr, 0.7, 0.7, 0.7); cairo_stroke(cr);
    auto positions = CalculateLayout(g_settings);
    for (const auto& pos : positions) {
        if (pos.page != g_preview_page) continue;
        draw_card_cairo(cr, pos.isFront ? g_front_path : g_back_path, pos.isFront ? g_front_xform : g_front_xform, offX + pos.x_mm * scale, offY + pos.y_mm * scale, g_settings.cardWidthMm * scale, g_settings.cardHeightMm * scale, g_settings.cardWidthMm);
    }
    return FALSE;
}

// Adjust Image Dialog
struct EditorState { std::string path; ImageTransform xform; GtkWidget *preview; };
static gboolean on_editor_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    EditorState *st = (EditorState*)data;
    cairo_set_source_rgb(cr, 1, 1, 1); cairo_paint(cr);
    draw_card_cairo(cr, st->path, st->xform, 20, 20, 85.6f * 3.0f, 53.98f * 3.0f, 85.6f);
    return FALSE;
}
static void on_slider_changed(GtkRange *range, gpointer data) {
    EditorState *st = (EditorState*)data; const char* name = gtk_widget_get_name(GTK_WIDGET(range));
    if (std::string(name) == "rot") st->xform.rotate = gtk_range_get_value(range);
    else if (std::string(name) == "zoom") st->xform.scale = gtk_range_get_value(range);
    else if (std::string(name) == "panx") st->xform.panX = gtk_range_get_value(range);
    else if (std::string(name) == "pany") st->xform.panY = gtk_range_get_value(range);
    gtk_widget_queue_draw(st->preview);
}

bool open_adjust_dialog(const std::string& path, ImageTransform& xform) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons("Adjust Image", GTK_WINDOW(window), GTK_DIALOG_MODAL, "_Cancel", GTK_RESPONSE_CANCEL, "_Save Changes", GTK_RESPONSE_ACCEPT, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 350, 550);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), vbox);
    EditorState st = { path, xform, gtk_drawing_area_new() }; gtk_widget_set_size_request(st.preview, 300, 200);
    g_signal_connect(st.preview, "draw", G_CALLBACK(on_editor_draw), &st); gtk_box_pack_start(GTK_BOX(vbox), st.preview, FALSE, FALSE, 0);
    auto add_slider = [&](const char* label, const char* name, float min, float max, float val) {
        gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(label), FALSE, FALSE, 0);
        GtkWidget *s = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, 0.01);
        gtk_widget_set_name(s, name); gtk_range_set_value(GTK_RANGE(s), val);
        g_signal_connect(s, "value-changed", G_CALLBACK(on_slider_changed), &st);
        gtk_box_pack_start(GTK_BOX(vbox), s, FALSE, FALSE, 0);
    };
    add_slider("Rotate:", "rot", 0, 360, xform.rotate); add_slider("Zoom:", "zoom", 0.1, 5.0, xform.scale);
    add_slider("Pan X:", "panx", -100, 100, xform.panX); add_slider("Pan Y:", "pany", -100, 100, xform.panY);
    gtk_widget_show_all(dlg); int res = gtk_dialog_run(GTK_DIALOG(dlg));
    if (res == GTK_RESPONSE_ACCEPT) { xform = st.xform; gtk_widget_destroy(dlg); return true; }
    gtk_widget_destroy(dlg); return false;
}

static void on_front_browse(GtkWidget *btn, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Front Image", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        ImageTransform temp = g_front_xform; if (open_adjust_dialog(path, temp)) { g_front_path = path; g_front_xform = temp; update_ui_from_settings(); gtk_widget_queue_draw(preview_area); }
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

static void on_back_browse(GtkWidget *btn, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Back Image", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        ImageTransform temp = g_back_xform; if (open_adjust_dialog(path, temp)) { g_back_path = path; g_back_xform = temp; update_ui_from_settings(); gtk_widget_queue_draw(preview_area); }
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

static void on_export_workspace(GtkMenuItem *item, gpointer data) {
    read_ui_settings(); GtkWidget *dialog = gtk_file_chooser_dialog_new("Export Workspace", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::ofstream f(path); if (f.is_open()) {
            f << g_front_path << "|" << g_front_xform.rotate << "|" << g_front_xform.scale << "|" << g_front_xform.panX << "|" << g_front_xform.panY << "\n";
            f << g_back_path << "|" << g_back_xform.rotate << "|" << g_back_xform.scale << "|" << g_back_xform.panX << "|" << g_back_xform.panY << "\n";
            f << g_settings.paperWidthMm << "|" << g_settings.paperHeightMm << "|" << g_settings.marginLeft << "|" << g_settings.marginRight << "|" << g_settings.marginTop << "|" << g_settings.marginBottom << "|" << (g_settings.isLandscape ? 1 : 0) << "|" << (g_settings.duplex ? 1 : 0) << "|" << g_settings.flipEdge << "|" << g_settings.cardWidthMm << "|" << g_settings.cardHeightMm << "\n";
            f.close();
        } g_free(path);
    } gtk_widget_destroy(dialog);
}

static void on_import_workspace(GtkMenuItem *item, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Import Workspace", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::ifstream f(path); if (f.is_open()) {
            std::string line; auto parse = [&](std::string& l, std::string& p, ImageTransform& x) {
                size_t pos = l.find("|"); p = l.substr(0, pos); l.erase(0, pos+1);
                pos = l.find("|"); x.rotate = std::stof(l.substr(0, pos)); l.erase(0, pos+1);
                pos = l.find("|"); x.scale = std::stof(l.substr(0, pos)); l.erase(0, pos+1);
                pos = l.find("|"); x.panX = std::stof(l.substr(0, pos)); l.erase(0, pos+1); x.panY = std::stof(l);
            };
            std::getline(f, line); parse(line, g_front_path, g_front_xform); std::getline(f, line); parse(line, g_back_path, g_back_xform);
            int iL, iD; char sep; f >> g_settings.paperWidthMm >> sep >> g_settings.paperHeightMm >> sep >> g_settings.marginLeft >> sep >> g_settings.marginRight >> sep >> g_settings.marginTop >> sep >> g_settings.marginBottom >> sep >> iL >> sep >> iD >> sep >> g_settings.flipEdge >> sep >> g_settings.cardWidthMm >> sep >> g_settings.cardHeightMm;
            g_settings.isLandscape = (iL != 0); g_settings.duplex = (iD != 0); f.close(); update_ui_from_settings(); gtk_widget_queue_draw(preview_area);
        } g_free(path);
    } gtk_widget_destroy(dialog);
}

static void on_ui_changed(GtkWidget *widget, gpointer data) { gtk_widget_queue_draw(preview_area); }
static void on_def_size_toggled(GtkToggleButton *btn, gpointer data) { gboolean active = gtk_toggle_button_get_active(btn); gtk_widget_set_sensitive(card_w_entry, !active); gtk_widget_set_sensitive(card_h_entry, !active); on_ui_changed(NULL, NULL); }
static void on_about(GtkWidget *btn, gpointer data) { GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "ID Card Print\nCreated by Maragung"); gtk_dialog_run(GTK_DIALOG(d)); gtk_widget_destroy(d); }

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL); gtk_window_set_title(GTK_WINDOW(window), "ID Card Print"); gtk_window_set_default_size(GTK_WINDOW(window), 1000, 800);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    GtkWidget *vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_container_add(GTK_CONTAINER(window), vbox_main);
    GtkWidget *menu_bar = gtk_menu_bar_new(); gtk_box_pack_start(GTK_BOX(vbox_main), menu_bar, FALSE, FALSE, 0);
    GtkWidget *file_it = gtk_menu_item_new_with_label("File"); gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_it);
    GtkWidget *file_menu = gtk_menu_new(); gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_it), file_menu);
    GtkWidget *imp_it = gtk_menu_item_new_with_label("Import"); g_signal_connect(imp_it, "activate", G_CALLBACK(on_import_workspace), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), imp_it);
    GtkWidget *exp_it = gtk_menu_item_new_with_label("Export"); g_signal_connect(exp_it, "activate", G_CALLBACK(on_export_workspace), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), exp_it);
    GtkWidget *theme_it = gtk_menu_item_new_with_label("Theme"); gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), theme_it);
    GtkWidget *theme_menu = gtk_menu_new(); gtk_menu_item_set_submenu(GTK_MENU_ITEM(theme_it), theme_menu);
    const char* themes[] = {"Light", "Dark", "Azure"};
    for(int i=0; i<3; ++i) { GtkWidget *it = gtk_menu_item_new_with_label(themes[i]); g_signal_connect(it, "activate", G_CALLBACK(on_theme_change), GINT_TO_POINTER(i)); gtk_menu_shell_append(GTK_MENU_SHELL(theme_menu), it); }
    GtkWidget *about_it = gtk_menu_item_new_with_label("About"); g_signal_connect(about_it, "activate", G_CALLBACK(on_about), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), about_it);

    GtkWidget *hbox_body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); gtk_container_set_border_width(GTK_CONTAINER(hbox_body), 5); gtk_box_pack_start(GTK_BOX(vbox_main), hbox_body, TRUE, TRUE, 0);
    
    GtkWidget *vbox_left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2); gtk_widget_set_size_request(vbox_left, 180, -1); gtk_box_pack_start(GTK_BOX(hbox_body), vbox_left, FALSE, FALSE, 0);
    GtkWidget *vbox_settings = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4); gtk_container_set_border_width(GTK_CONTAINER(vbox_settings), 5); gtk_box_pack_start(GTK_BOX(vbox_left), vbox_settings, TRUE, TRUE, 0);

    auto add_field = [&](const char* label, GtkWidget* widget) {
        GtkWidget *l = gtk_label_new(label); gtk_label_set_xalign(GTK_LABEL(l), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox_settings), l, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox_settings), widget, FALSE, FALSE, 0);
    };

    GtkWidget *btn_f = gtk_button_new_with_label("SELECT FRONT SIDE"); g_signal_connect(btn_f, "clicked", G_CALLBACK(on_front_browse), NULL); gtk_box_pack_start(GTK_BOX(vbox_settings), btn_f, FALSE, FALSE, 0);
    front_label = gtk_label_new("None"); gtk_label_set_ellipsize(GTK_LABEL(front_label), PANGO_ELLIPSIZE_END); gtk_label_set_xalign(GTK_LABEL(front_label), 0.0); gtk_box_pack_start(GTK_BOX(vbox_settings), front_label, FALSE, FALSE, 0);
    GtkWidget *btn_b = gtk_button_new_with_label("SELECT BACK SIDE"); g_signal_connect(btn_b, "clicked", G_CALLBACK(on_back_browse), NULL); gtk_box_pack_start(GTK_BOX(vbox_settings), btn_b, FALSE, FALSE, 0);
    back_label = gtk_label_new("None"); gtk_label_set_ellipsize(GTK_LABEL(back_label), PANGO_ELLIPSIZE_END); gtk_label_set_xalign(GTK_LABEL(back_label), 0.0); gtk_box_pack_start(GTK_BOX(vbox_settings), back_label, FALSE, FALSE, 0);
    
    def_size_check = gtk_check_button_new_with_label("Use Default Size"); gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(def_size_check), TRUE); g_signal_connect(def_size_check, "toggled", G_CALLBACK(on_def_size_toggled), NULL); gtk_box_pack_start(GTK_BOX(vbox_settings), def_size_check, FALSE, FALSE, 0);
    GtkWidget *hbox_cs = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2); gtk_box_pack_start(GTK_BOX(vbox_settings), hbox_cs, FALSE, FALSE, 0);
    card_w_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(card_w_entry), "85.60"); gtk_widget_set_sensitive(card_w_entry, FALSE); gtk_box_pack_start(GTK_BOX(hbox_cs), card_w_entry, TRUE, TRUE, 0);
    card_h_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(card_h_entry), "53.98"); gtk_widget_set_sensitive(card_h_entry, FALSE); gtk_box_pack_start(GTK_BOX(hbox_cs), card_h_entry, TRUE, TRUE, 0);
    
    orient_combo = gtk_combo_box_text_new(); gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(orient_combo), "Portrait"); gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(orient_combo), "Landscape"); gtk_combo_box_set_active(GTK_COMBO_BOX(orient_combo), 0); g_signal_connect(orient_combo, "changed", G_CALLBACK(on_ui_changed), NULL);
    add_field("Orientation:", orient_combo);
    paper_combo = gtk_combo_box_text_new(); gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(paper_combo), "Letter"); gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(paper_combo), "A4"); gtk_combo_box_set_active(GTK_COMBO_BOX(paper_combo), 1); g_signal_connect(paper_combo, "changed", G_CALLBACK(on_ui_changed), NULL);
    add_field("Paper Size:", paper_combo);
    
    GtkWidget *l_m = gtk_label_new("Margins (L/R/T/B):"); gtk_label_set_xalign(GTK_LABEL(l_m), 0.0); gtk_box_pack_start(GTK_BOX(vbox_settings), l_m, FALSE, FALSE, 0);
    GtkWidget *hbox_m1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2); gtk_box_pack_start(GTK_BOX(vbox_settings), hbox_m1, FALSE, FALSE, 0);
    ml_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(ml_entry), "10.0"); gtk_box_pack_start(GTK_BOX(hbox_m1), ml_entry, TRUE, TRUE, 0);
    mr_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(mr_entry), "10.0"); gtk_box_pack_start(GTK_BOX(hbox_m1), mr_entry, TRUE, TRUE, 0);
    GtkWidget *hbox_m2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2); gtk_box_pack_start(GTK_BOX(vbox_settings), hbox_m2, FALSE, FALSE, 0);
    mt_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(mt_entry), "10.0"); gtk_box_pack_start(GTK_BOX(hbox_m2), mt_entry, TRUE, TRUE, 0);
    mb_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(mb_entry), "10.0"); gtk_box_pack_start(GTK_BOX(hbox_m2), mb_entry, TRUE, TRUE, 0);
    g_signal_connect(ml_entry, "changed", G_CALLBACK(on_ui_changed), NULL); g_signal_connect(mr_entry, "changed", G_CALLBACK(on_ui_changed), NULL); g_signal_connect(mt_entry, "changed", G_CALLBACK(on_ui_changed), NULL); g_signal_connect(mb_entry, "changed", G_CALLBACK(on_ui_changed), NULL);
    
    duplex_check = gtk_check_button_new_with_label("Double Sided"); gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(duplex_check), TRUE); g_signal_connect(duplex_check, "toggled", G_CALLBACK(on_ui_changed), NULL); gtk_box_pack_start(GTK_BOX(vbox_settings), duplex_check, FALSE, FALSE, 0);
    edge_combo = gtk_combo_box_text_new(); gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(edge_combo), "Long Edge Flip"); gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(edge_combo), "Short Edge Flip"); gtk_combo_box_set_active(GTK_COMBO_BOX(edge_combo), 0); g_signal_connect(edge_combo, "changed", G_CALLBACK(on_ui_changed), NULL); gtk_box_pack_start(GTK_BOX(vbox_settings), edge_combo, FALSE, FALSE, 0);
    
    copies_entry = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(copies_entry), "1"); g_signal_connect(copies_entry, "changed", G_CALLBACK(on_ui_changed), NULL);
    add_field("Print Copies:", copies_entry);
    
    GtkWidget *btn_print = gtk_button_new_with_label("PRINT NOW"); gtk_box_pack_start(GTK_BOX(vbox_settings), btn_print, FALSE, FALSE, 10);

    GtkWidget *vbox_right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); gtk_box_pack_start(GTK_BOX(hbox_body), vbox_right, TRUE, TRUE, 0);
    preview_area = gtk_drawing_area_new(); gtk_widget_set_hexpand(preview_area, TRUE); gtk_widget_set_vexpand(preview_area, TRUE); g_signal_connect(preview_area, "draw", G_CALLBACK(on_draw), NULL); gtk_box_pack_start(GTK_BOX(vbox_right), preview_area, TRUE, TRUE, 0);
    GtkWidget *hbox_pg = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); gtk_box_pack_start(GTK_BOX(vbox_right), hbox_pg, FALSE, FALSE, 0);
    GtkWidget *btn_p1 = gtk_button_new_with_label("PAGE 1"); g_signal_connect(btn_p1, "clicked", G_CALLBACK(+[](GtkWidget* b, gpointer d){ g_preview_page=0; gtk_widget_queue_draw(preview_area); }), NULL); gtk_box_pack_start(GTK_BOX(hbox_pg), btn_p1, TRUE, TRUE, 0);
    GtkWidget *btn_p2 = gtk_button_new_with_label("PAGE 2"); g_signal_connect(btn_p2, "clicked", G_CALLBACK(+[](GtkWidget* b, gpointer d){ g_preview_page=1; gtk_widget_queue_draw(preview_area); }), NULL); gtk_box_pack_start(GTK_BOX(hbox_pg), btn_p2, TRUE, TRUE, 0);
    
    apply_theme(); gtk_widget_show_all(window); gtk_main(); return 0;
}
