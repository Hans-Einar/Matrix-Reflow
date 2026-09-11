#include "settings.h"
#include "preview.h"
#include <gtk/gtk.h>
#include <glib-unix.h>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Control {const reflow::SettingField* field;GtkWidget* widget;};
class SettingsApp {
    reflow::Profiles profiles_;
    reflow::Preview preview_;
    std::string path_=reflow::settings_path();
    GtkWidget *window_=nullptr,*profiles_combo_=nullptr,*name_=nullptr,*status_=nullptr,*stop_=nullptr;
    GtkWidget *main_color_=nullptr,*glitch_color_=nullptr;
    std::vector<Control> controls_;
    bool loading_=false,dirty_=false,was_running_=false;
    guint timer_=0,term_=0,int_=0;
    reflow::Profile& current() {return profiles_.profiles[profiles_.selected-1];}
    void status(const std::string& text) {gtk_label_set_text(GTK_LABEL(status_),text.c_str());}
    void dirty() {dirty_=true;gtk_window_set_title(GTK_WINDOW(window_),"Matrix Reflow Settings • Unsaved");status("Unsaved changes — Preview tries them without saving.");}
    void collect() {
        if(loading_) return;
        current().name=gtk_entry_get_text(GTK_ENTRY(name_));
        for(const auto& c:controls_) {
            const double value=c.field->type==reflow::FieldType::Boolean?
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c.widget)):gtk_spin_button_get_value(GTK_SPIN_BUTTON(c.widget));
            c.field->put(current().settings,value);
        }
        GdkRGBA a{},b{};gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(main_color_),&a);gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(glitch_color_),&b);
        auto& s=current().settings.rain;
        s.mainColorR=a.red;s.mainColorG=a.green;s.mainColorB=a.blue;
        s.glitchColorR=b.red;s.glitchColorG=b.green;s.glitchColorB=b.blue;
        reflow::validate_settings(current().settings);
    }
    void refresh_names() {
        const bool before=loading_;loading_=true;
        gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(profiles_combo_));
        for(size_t i=0;i<5;++i) {
            const auto label=std::to_string(i+1)+" · "+profiles_.profiles[i].name;
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(profiles_combo_),label.c_str());
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(profiles_combo_),profiles_.selected-1);loading_=before;
    }
    void load() {
        loading_=true;
        gtk_entry_set_text(GTK_ENTRY(name_),current().name.c_str());
        for(const auto& c:controls_) {
            const auto v=c.field->get(current().settings);
            if(c.field->type==reflow::FieldType::Boolean) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c.widget),v!=0);
            else gtk_spin_button_set_value(GTK_SPIN_BUTTON(c.widget),v);
        }
        const auto& s=current().settings.rain;
        GdkRGBA a{s.mainColorR,s.mainColorG,s.mainColorB,1},b{s.glitchColorR,s.glitchColorG,s.glitchColorB,1};
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(main_color_),&a);gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(glitch_color_),&b);
        refresh_names();loading_=false;
    }
    template<class F> void attempt(F action) {try {action();}catch(const std::exception& e){status(std::string("Error: ")+e.what());}}
    static void changed(GtkWidget*,gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);if(app.loading_) return;
        app.attempt([&]{app.collect();app.dirty();});
    }
    static void selected(GtkComboBox* combo,gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);if(app.loading_) return;
        const int slot=gtk_combo_box_get_active(combo);if(slot<0) return;
        app.attempt([&]{app.profiles_.selected=slot+1;app.load();app.dirty();});
    }
    static void save(GtkButton*,gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);
        app.attempt([&]{app.collect();reflow::save_profiles(app.profiles_,app.path_);app.refresh_names();app.dirty_=false;
            gtk_window_set_title(GTK_WINDOW(app.window_),"Matrix Reflow Settings");app.status("Saved all five profiles. This profile is now active.");});
    }
    static void reset(GtkButton*,gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);app.current().settings=reflow::Settings{};app.load();app.dirty();
    }
    static void preview(GtkButton*,gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);
        app.attempt([&]{app.collect();app.preview_.start(reflow::renderer_executable(),reflow::settings_arguments(app.current().settings));
            app.was_running_=true;gtk_widget_set_sensitive(app.stop_,TRUE);app.status("Preview uses current choices. Close it with Escape or Stop preview.");});
    }
    static void stop(GtkButton*,gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);app.preview_.stop();app.was_running_=false;gtk_widget_set_sensitive(app.stop_,FALSE);app.status("Preview stopped.");
    }
    static gboolean tick(gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);
        const bool running=app.preview_.poll();gtk_widget_set_sensitive(app.stop_,running);
        if(app.was_running_ && !running) app.status(app.preview_.failed()?"Preview failed. See terminal output for details.":"Preview closed.");
        app.was_running_=running;return G_SOURCE_CONTINUE;
    }
    static gboolean close(GtkWidget*,GdkEvent*,gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);
        if(app.dirty_) {
            GtkWidget* dialog=gtk_message_dialog_new(GTK_WINDOW(app.window_),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_NONE,"Save changes before closing?");
            gtk_dialog_add_buttons(GTK_DIALOG(dialog),"Cancel",GTK_RESPONSE_CANCEL,"Discard",GTK_RESPONSE_REJECT,"Save",GTK_RESPONSE_ACCEPT,nullptr);
            const auto response=gtk_dialog_run(GTK_DIALOG(dialog));gtk_widget_destroy(dialog);
            if(response==GTK_RESPONSE_ACCEPT) {save(nullptr,&app);if(app.dirty_) return TRUE;}
            else if(response!=GTK_RESPONSE_REJECT) return TRUE;
        }
        gtk_widget_destroy(app.window_);return TRUE;
    }
    static void destroy(GtkWidget*,gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);
        if(app.timer_) {g_source_remove(app.timer_);app.timer_=0;}
        if(app.term_) g_source_remove(app.term_);
        if(app.int_) g_source_remove(app.int_);
        app.term_=app.int_=0;
        app.preview_.stop();gtk_main_quit();
    }
    static gboolean terminate(gpointer data) {
        auto& app=*static_cast<SettingsApp*>(data);gtk_widget_destroy(app.window_);return G_SOURCE_REMOVE;
    }
    GtkWidget* button(const char* text,GCallback callback) {
        auto* w=gtk_button_new_with_label(text);g_signal_connect(w,"clicked",callback,this);return w;
    }
public:
    SettingsApp() {
        profiles_=reflow::load_profiles(path_);
        window_=gtk_window_new(GTK_WINDOW_TOPLEVEL);gtk_window_set_title(GTK_WINDOW(window_),"Matrix Reflow Settings");
        gtk_window_set_default_size(GTK_WINDOW(window_),660,720);
        g_signal_connect(window_,"delete-event",G_CALLBACK(close),this);g_signal_connect(window_,"destroy",G_CALLBACK(destroy),this);
        auto* box=gtk_box_new(GTK_ORIENTATION_VERTICAL,14);gtk_container_set_border_width(GTK_CONTAINER(box),18);gtk_container_add(GTK_CONTAINER(window_),box);
        auto* title=gtk_label_new(nullptr);gtk_label_set_markup(GTK_LABEL(title),"<span size='x-large' weight='bold'>Matrix Reflow</span>");gtk_widget_set_halign(title,GTK_ALIGN_START);gtk_box_pack_start(GTK_BOX(box),title,FALSE,FALSE,0);
        auto* top=gtk_grid_new();gtk_grid_set_column_spacing(GTK_GRID(top),12);gtk_grid_set_row_spacing(GTK_GRID(top),8);
        profiles_combo_=gtk_combo_box_text_new();name_=gtk_entry_new();gtk_entry_set_max_length(GTK_ENTRY(name_),128);
        gtk_grid_attach(GTK_GRID(top),gtk_label_new("Active profile"),0,0,1,1);gtk_grid_attach(GTK_GRID(top),profiles_combo_,1,0,1,1);
        gtk_grid_attach(GTK_GRID(top),gtk_label_new("Profile name"),0,1,1,1);gtk_grid_attach(GTK_GRID(top),name_,1,1,1,1);gtk_widget_set_hexpand(name_,TRUE);
        gtk_box_pack_start(GTK_BOX(box),top,FALSE,FALSE,0);
        g_signal_connect(profiles_combo_,"changed",G_CALLBACK(selected),this);g_signal_connect(name_,"changed",G_CALLBACK(changed),this);
        auto* notebook=gtk_notebook_new();gtk_box_pack_start(GTK_BOX(box),notebook,TRUE,TRUE,0);
        for(const char* group:{"Rain","Scene","Effects","Colors"}) {
            auto* grid=gtk_grid_new();gtk_container_set_border_width(GTK_CONTAINER(grid),16);gtk_grid_set_column_spacing(GTK_GRID(grid),18);gtk_grid_set_row_spacing(GTK_GRID(grid),10);
            int row=0;
            for(const auto& field:reflow::setting_fields()) {
                if(std::string(group)!=field.group || std::string(group)=="Colors") continue;
                GtkWidget* control;
                if(field.type==reflow::FieldType::Boolean) {
                    control=gtk_check_button_new_with_label(field.label);gtk_grid_attach(GTK_GRID(grid),control,0,row,3,1);
                    g_signal_connect(control,"toggled",G_CALLBACK(changed),this);
                } else {
                    const double step=field.type==reflow::FieldType::Integer?1:.01;
                    auto* adj=gtk_adjustment_new(field.get(reflow::Settings{}),field.low,field.high,step,step*10,0);
                    control=gtk_spin_button_new(adj,step,field.type==reflow::FieldType::Integer?0:3);
                    gtk_entry_set_width_chars(GTK_ENTRY(control),7);
                    auto* label=gtk_label_new(field.label);gtk_widget_set_halign(label,GTK_ALIGN_START);
                    gtk_label_set_mnemonic_widget(GTK_LABEL(label),control);
                    gtk_grid_attach(GTK_GRID(grid),label,0,row,1,1);
                    auto* slider=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,adj);gtk_scale_set_draw_value(GTK_SCALE(slider),FALSE);gtk_widget_set_hexpand(slider,TRUE);
                    gtk_grid_attach(GTK_GRID(grid),slider,1,row,1,1);gtk_grid_attach(GTK_GRID(grid),control,2,row,1,1);
                    atk_object_set_name(gtk_widget_get_accessible(control),field.label);
                    g_signal_connect(control,"value-changed",G_CALLBACK(changed),this);
                }
                controls_.push_back({&field,control});++row;
            }
            if(std::string(group)=="Colors") {
                main_color_=gtk_color_button_new();glitch_color_=gtk_color_button_new();
                g_object_set(main_color_,"show-editor",TRUE,nullptr);g_object_set(glitch_color_,"show-editor",TRUE,nullptr);
                gtk_color_button_set_title(GTK_COLOR_BUTTON(main_color_),"Main rain color");gtk_color_button_set_title(GTK_COLOR_BUTTON(glitch_color_),"Glitch color");
                gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Main rain"),0,0,1,1);gtk_grid_attach(GTK_GRID(grid),main_color_,1,0,1,1);
                gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Glitch"),0,1,1,1);gtk_grid_attach(GTK_GRID(grid),glitch_color_,1,1,1,1);
                atk_object_set_name(gtk_widget_get_accessible(main_color_),"Main rain color");atk_object_set_name(gtk_widget_get_accessible(glitch_color_),"Glitch color");
                g_signal_connect(main_color_,"color-set",G_CALLBACK(changed),this);g_signal_connect(glitch_color_,"color-set",G_CALLBACK(changed),this);
            }
            auto* scroll=gtk_scrolled_window_new(nullptr,nullptr);gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);gtk_container_add(GTK_CONTAINER(scroll),grid);
            gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scroll,gtk_label_new(group));
        }
        status_=gtk_label_new("Choose a profile, adjust settings, then Preview or Save.");gtk_label_set_line_wrap(GTK_LABEL(status_),TRUE);gtk_label_set_xalign(GTK_LABEL(status_),0);gtk_widget_set_size_request(status_,-1,40);gtk_box_pack_start(GTK_BOX(box),status_,FALSE,FALSE,0);
        auto* actions=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);gtk_box_pack_start(GTK_BOX(box),actions,FALSE,FALSE,0);
        gtk_box_pack_start(GTK_BOX(actions),button("Reset profile",G_CALLBACK(reset)),FALSE,FALSE,0);
        gtk_box_pack_end(GTK_BOX(actions),button("Save profiles",G_CALLBACK(save)),FALSE,FALSE,0);
        stop_=button("Stop preview",G_CALLBACK(stop));gtk_widget_set_sensitive(stop_,FALSE);gtk_box_pack_end(GTK_BOX(actions),stop_,FALSE,FALSE,0);
        gtk_box_pack_end(GTK_BOX(actions),button("Preview",G_CALLBACK(preview)),FALSE,FALSE,0);
        load();timer_=g_timeout_add(200,tick,this);
        term_=g_unix_signal_add(SIGTERM,terminate,this);int_=g_unix_signal_add(SIGINT,terminate,this);
        gtk_widget_show_all(window_);
    }
};
}
int main(int argc,char** argv) {
    if(argc==2 && std::string(argv[1])=="--help") {std::cout<<"Matrix Reflow Settings\nEdit five profiles, preview unsaved choices, and save to "<<reflow::settings_path()<<"\n";return 0;}
    if(argc>1) {std::cerr<<"Use matrix-reflow-settings without arguments (or --help).\n";return 1;}
    if(!gtk_init_check(&argc,&argv)) {std::cerr<<"matrix-reflow-settings: cannot open display\n";return 1;}
    try {SettingsApp app;gtk_main();return 0;}
    catch(const std::exception& e) {
        std::cerr<<"matrix-reflow-settings: "<<e.what()<<'\n';
        auto* dialog=gtk_message_dialog_new(nullptr,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,"Cannot open settings: %s",e.what());
        gtk_dialog_run(GTK_DIALOG(dialog));gtk_widget_destroy(dialog);return 1;
    }
}
