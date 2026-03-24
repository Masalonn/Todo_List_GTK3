#include <gtk/gtk.h>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

using namespace rapidjson;

GtkWidget *window_main;
GtkWidget *entry_search;
GtkWidget *combo_sort;
GtkWidget *entry_text;
GtkWidget *combo_priority;
GtkWidget *list_box;
GtkWidget *btn_add;
GtkWidget *btn_del;

void apply_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css =
        "label.prio-high { color: #e74c3c; font-weight: bold; }"
        "label.prio-mid { color: #e67e22; }"
        "label.prio-low { color: #7f8c8d; }"
        "window { font-family: 'Segoe UI', sans-serif; }"
        "entry { padding: 6px; border-radius: 4px; }"
        "button { padding: 6px; font-weight: bold; }";

    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

void add_row(const char *text, bool active, int priority) {
    if (g_strcmp0(text, "") == 0) return;

    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *check_btn = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_btn), active);

    GtkWidget *label_text = gtk_label_new(text);
    gtk_widget_set_halign(label_text, GTK_ALIGN_START);

    const char *prio_text = (priority == 2) ? "Wazne" : (priority == 1 ? "Srednie" : "Niskie");
    GtkWidget *label_prio = gtk_label_new(prio_text);

    GtkStyleContext *context = gtk_widget_get_style_context(label_prio);
    if (priority == 2) gtk_style_context_add_class(context, "prio-high");
    else if (priority == 1) gtk_style_context_add_class(context, "prio-mid");
    else gtk_style_context_add_class(context, "prio-low");

    gtk_box_pack_start(GTK_BOX(row_box), check_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_box), label_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row_box), label_prio, FALSE, FALSE, 10);

    gtk_list_box_insert(GTK_LIST_BOX(list_box), row_box, -1);
    gtk_widget_show_all(list_box);
}

void save_data() {
    Document d;
    d.SetArray();
    Document::AllocatorType& allocator = d.GetAllocator();

    GList *children = gtk_container_get_children(GTK_CONTAINER(list_box));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        GtkWidget *box = gtk_bin_get_child(GTK_BIN(row));
        GList *box_children = gtk_container_get_children(GTK_CONTAINER(box));

        GtkWidget *check = GTK_WIDGET(g_list_nth_data(box_children, 0));
        GtkWidget *l_text = GTK_WIDGET(g_list_nth_data(box_children, 1));
        GtkWidget *l_prio = GTK_WIDGET(g_list_nth_data(box_children, 2));

        bool is_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
        const char *txt = gtk_label_get_text(GTK_LABEL(l_text));
        const char *prio_str = gtk_label_get_text(GTK_LABEL(l_prio));

        int prio_val = 0;
        if (g_strcmp0(prio_str, "Wazne") == 0) prio_val = 2;
        else if (g_strcmp0(prio_str, "Srednie") == 0) prio_val = 1;

        Value obj(kObjectType);
        obj.AddMember("status", is_active, allocator);
        Value v_txt;
        v_txt.SetString(txt, allocator);
        obj.AddMember("tekst", v_txt, allocator);
        obj.AddMember("priorytet", prio_val, allocator);

        d.PushBack(obj, allocator);
        g_list_free(box_children);
    }
    g_list_free(children);

    std::ofstream ofs("data.json");
    OStreamWrapper osw(ofs);
    Writer<OStreamWrapper> writer(osw);
    d.Accept(writer);
}

void load_data() {
    std::ifstream ifs("data.json");
    if (!ifs.is_open()) return;

    IStreamWrapper isw(ifs);
    Document d;
    d.ParseStream(isw);

    if (d.IsArray()) {
        for (SizeType i = 0; i < d.Size(); i++) {
            const Value& obj = d[i];
            if (obj.HasMember("tekst")) {
                bool status = obj.HasMember("status") ? obj["status"].GetBool() : false;
                const char* text = obj["tekst"].GetString();
                int prio = obj.HasMember("priorytet") ? obj["priorytet"].GetInt() : 0;
                add_row(text, status, prio);
            }
        }
    }
}

int get_prio_val(const char* p) {
    if (g_strcmp0(p, "Wazne") == 0) return 2;
    if (g_strcmp0(p, "Srednie") == 0) return 1;
    return 0;
}

int sort_func(GtkListBoxRow *row1, GtkListBoxRow *row2, gpointer user_data) {
    int mode = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_sort));

    GtkWidget *box1 = gtk_bin_get_child(GTK_BIN(row1));
    GList *c1 = gtk_container_get_children(GTK_CONTAINER(box1));
    GtkWidget *l_text1 = GTK_WIDGET(g_list_nth_data(c1, 1));
    GtkWidget *l_prio1 = GTK_WIDGET(g_list_nth_data(c1, 2));

    GtkWidget *box2 = gtk_bin_get_child(GTK_BIN(row2));
    GList *c2 = gtk_container_get_children(GTK_CONTAINER(box2));
    GtkWidget *l_text2 = GTK_WIDGET(g_list_nth_data(c2, 1));
    GtkWidget *l_prio2 = GTK_WIDGET(g_list_nth_data(c2, 2));

    const char *t1 = gtk_label_get_text(GTK_LABEL(l_text1));
    const char *p1 = gtk_label_get_text(GTK_LABEL(l_prio1));
    const char *t2 = gtk_label_get_text(GTK_LABEL(l_text2));
    const char *p2 = gtk_label_get_text(GTK_LABEL(l_prio2));

    int result = 0;

    if (mode == 0) { // sortowanie wazne-niskie
        int v1 = get_prio_val(p1);
        int v2 = get_prio_val(p2);
        result = v2 - v1;
    } else if (mode == 1) { //  niskie-wazne
        int v1 = get_prio_val(p1);
        int v2 = get_prio_val(p2);
        result = v1 - v2;
    } else if (mode == 2) { //  A-Z
        std::string s1 = t1;
        std::string s2 = t2;
        std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
        std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
        result = s1.compare(s2);
    } else if (mode == 3) { //  Z-A
        std::string s1 = t1;
        std::string s2 = t2;
        std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
        std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
        result = s2.compare(s1);
    }

    g_list_free(c1);
    g_list_free(c2);
    return result;
}

int filter_func(GtkListBoxRow *row, void *user_data) {
    GtkWidget *box = gtk_bin_get_child(GTK_BIN(row));
    GList *children = gtk_container_get_children(GTK_CONTAINER(box));
    GtkWidget *label = GTK_WIDGET(g_list_nth_data(children, 1));
    const char *text = gtk_label_get_text(GTK_LABEL(label));
    const char *search = gtk_entry_get_text(GTK_ENTRY(entry_search));

    std::string s_text = text;
    std::string s_search = search;
    std::transform(s_text.begin(), s_text.end(), s_text.begin(), ::tolower);
    std::transform(s_search.begin(), s_search.end(), s_search.begin(), ::tolower);

    g_list_free(children);

    if (s_search.empty()) return true;
    return s_text.find(s_search) != std::string::npos;
}

void on_search_changed(GtkSearchEntry *entry, gpointer data) {
    gtk_list_box_invalidate_filter(GTK_LIST_BOX(list_box));
}

void on_sort_changed(GtkComboBox *combo, gpointer data) {
    gtk_list_box_invalidate_sort(GTK_LIST_BOX(list_box));
}

void on_btn_add_clicked(GtkButton *btn, gpointer data) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry_text));
    int priority = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_priority));
    if(priority == -1) priority = 0;

    add_row(text, false, priority);
    gtk_entry_set_text(GTK_ENTRY(entry_text), "");
    save_data();
}

void on_btn_del_clicked(GtkButton *btn, gpointer data) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(list_box));
    bool changed = false;
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        GtkWidget *box = gtk_bin_get_child(GTK_BIN(row));
        GList *box_children = gtk_container_get_children(GTK_CONTAINER(box));
        GtkWidget *check = GTK_WIDGET(g_list_nth_data(box_children, 0));

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check))) {
            gtk_widget_destroy(row);
            changed = true;
        }
        g_list_free(box_children);
    }
    g_list_free(children);
    if (changed) save_data();
}

extern "C" void on_window_main_destroy() {
    save_data();
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    apply_css();

    GtkBuilder *builder = gtk_builder_new();
    GError *error = NULL;

    if (gtk_builder_add_from_file(builder, "projekt.glade", &error) == 0) {
        g_printerr("%s\n", error->message);
        return 1;
    }

    window_main = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
    entry_search = GTK_WIDGET(gtk_builder_get_object(builder, "entry_search"));
    combo_sort = GTK_WIDGET(gtk_builder_get_object(builder, "combo_sort"));
    entry_text = GTK_WIDGET(gtk_builder_get_object(builder, "entry_text"));
    combo_priority = GTK_WIDGET(gtk_builder_get_object(builder, "combo_priority"));
    list_box = GTK_WIDGET(gtk_builder_get_object(builder, "list_box"));
    btn_add = GTK_WIDGET(gtk_builder_get_object(builder, "btn_add"));
    btn_del = GTK_WIDGET(gtk_builder_get_object(builder, "btn_del"));

    gtk_list_box_set_filter_func(GTK_LIST_BOX(list_box), (GtkListBoxFilterFunc)filter_func, NULL, NULL);
    gtk_list_box_set_sort_func(GTK_LIST_BOX(list_box), (GtkListBoxSortFunc)sort_func, NULL, NULL);

    g_signal_connect(entry_search, "search-changed", G_CALLBACK(on_search_changed), NULL);
    g_signal_connect(combo_sort, "changed", G_CALLBACK(on_sort_changed), NULL);
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_btn_add_clicked), NULL);
    g_signal_connect(btn_del, "clicked", G_CALLBACK(on_btn_del_clicked), NULL);
    g_signal_connect(window_main, "destroy", G_CALLBACK(on_window_main_destroy), NULL);

    load_data();
    gtk_widget_show_all(window_main);
    gtk_main();

    return 0;
}
