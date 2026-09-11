#include "settings.h"
#include <glib.h>
#include <charconv>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>

namespace reflow {
const std::vector<SettingField>& setting_fields() {
#define FIELD(key,label,group,type,low,high,member) {key,label,group,FieldType::type,low,high, \
    [](const Settings& s)->double{return s.member;}, \
    [](Settings& s,double v){s.member=static_cast<decltype(s.member)>(v);}}
    static const std::vector<SettingField> fields = {
        FIELD("speed","Speed","Rain",Number,0,1,rain.speed),
        FIELD("density","Density","Rain",Number,.05,1,rain.density),
        FIELD("scale","Glyph size","Rain",Number,.1,2,rain.glyphScale),
        FIELD("length","Length bias","Rain",Number,0,1,rain.lengthBias),
        FIELD("mutation","Mutation rate","Rain",Number,0,1,rain.mutationRate),
        FIELD("binary","Binary characters","Rain",Boolean,0,1,rain.binaryMode),
        FIELD("column-gaps","Column gaps","Rain",Boolean,0,1,rain.columnGaps),
        FIELD("flip-x","Horizontal glyph flips","Rain",Boolean,0,1,rain.flipXEnabled),
        FIELD("flip-y","Vertical glyph flips","Rain",Boolean,0,1,rain.flipYEnabled),
        FIELD("easter-eggs","Clock easter eggs","Rain",Boolean,0,1,rain.easterEggs),
        FIELD("depth","Depth","Scene",Number,0,1.5,rain.depthAmount),
        FIELD("panning","Camera movement","Scene",Boolean,0,1,rain.panning),
        FIELD("camera-speed","Camera speed","Scene",Number,0,1,rain.cameraSpeed),
        FIELD("fog","Depth fog","Scene",Boolean,0,1,rain.fog),
        FIELD("textured","Textured glyphs","Scene",Boolean,0,1,rain.textured),
        FIELD("wireframe","Wireframe","Scene",Boolean,0,1,rain.wireframe),
        FIELD("extra-contrast-heads","Bright heads","Effects",Boolean,0,1,rain.extraContrastHeads),
        FIELD("bloom","Bloom","Effects",Boolean,0,1,rain.bloom),
        FIELD("bloom-strength","Bloom strength","Effects",Number,0,1,rain.bloomIntensity),
        FIELD("crt","CRT emulation","Effects",Boolean,0,1,rain.crtEmulation),
        FIELD("distortion","Glass distortion","Effects",Number,0,1,rain.crtDistort),
        FIELD("fps-limit","Frame limit","Effects",Integer,1,240,fps_limit),
        FIELD("main-red","Main red","Colors",Number,0,1,rain.mainColorR),
        FIELD("main-green","Main green","Colors",Number,0,1,rain.mainColorG),
        FIELD("main-blue","Main blue","Colors",Number,0,1,rain.mainColorB),
        FIELD("glitch-red","Glitch red","Colors",Number,0,1,rain.glitchColorR),
        FIELD("glitch-green","Glitch green","Colors",Number,0,1,rain.glitchColorG),
        FIELD("glitch-blue","Glitch blue","Colors",Number,0,1,rain.glitchColorB)
    };
#undef FIELD
    return fields;
}
const SettingField* setting_field(const std::string& key) {
    for (const auto& f : setting_fields()) if (key == f.key) return &f;
    return nullptr;
}
std::string setting_number(double v) {
    char text[64]; const auto r=std::to_chars(text,text+sizeof text,v);
    if(r.ec!=std::errc{}) throw std::runtime_error("Cannot format setting");
    return {text,r.ptr};
}
void set_setting(Settings& s,const std::string& key,const std::string& value) {
    const auto* f=setting_field(key);
    if(!f) throw std::runtime_error("Unknown setting: "+key);
    double v=0;const auto r=std::from_chars(value.data(),value.data()+value.size(),v);
    if(r.ec!=std::errc{} || r.ptr!=value.data()+value.size() || !std::isfinite(v) ||
       v<f->low || v>f->high || (f->type!=FieldType::Number && std::floor(v)!=v))
        throw std::runtime_error(key+" must be "+setting_number(f->low)+".."+setting_number(f->high)+
            (f->type==FieldType::Number?"":" (integer)")+": "+value);
    f->put(s,v);
}
void validate_settings(const Settings& s) {
    for(const auto& f:setting_fields()) { Settings test; set_setting(test,f.key,setting_number(f.get(s))); }
}
std::string effective_settings(const Settings& s) {
    validate_settings(s);std::string result;
    for(const auto& f:setting_fields()) result+=std::string(f.key)+"="+setting_number(f.get(s))+"\n";
    return result;
}
std::vector<std::string> settings_arguments(const Settings& s) {
    validate_settings(s);std::vector<std::string> args{"--no-config","--windowed"};
    for(const auto& f:setting_fields()) {
        args.push_back(std::string("--")+(f.type==FieldType::Boolean && !f.get(s)?"no-":"")+f.key);
        if(f.type!=FieldType::Boolean) args.push_back(setting_number(f.get(s)));
    }
    return args;
}
Profiles::Profiles() {
    for(size_t i=0;i<profiles.size();++i) profiles[i].name="Profile "+std::to_string(i+1);
}
std::string settings_path() {
    const char* xdg=g_getenv("XDG_CONFIG_HOME");
    const std::string base=xdg && g_path_is_absolute(xdg)?xdg:std::string(g_get_home_dir())+"/.config";
    return base+"/matrix-reflow/settings.ini";
}
namespace {
using KeyFile=std::unique_ptr<GKeyFile,decltype(&g_key_file_unref)>;
std::string take_error(GError* error) {
    std::string text=error?error->message:"Unknown configuration error";
    if(error) g_error_free(error);
    return text;
}
std::string read(GKeyFile* key,const char* group,const char* name) {
    GError* error=nullptr;gchar* value=g_key_file_get_string(key,group,name,&error);
    if(error) throw std::runtime_error(take_error(error));
    std::string result=value;g_free(value);return result;
}
void validate_profiles(const Profiles& p) {
    if(p.selected<1 || p.selected>5) throw std::runtime_error("Selected profile must be 1..5");
    for(const auto& profile:p.profiles) {
        if(profile.name.empty() || profile.name.size()>128 || !g_utf8_validate(profile.name.c_str(),-1,nullptr) ||
           profile.name.find_first_of("\r\n")!=std::string::npos)
            throw std::runtime_error("Profile name must be 1..128 UTF-8 bytes on one line");
        validate_settings(profile.settings);
    }
}
}
Profiles load_profiles(const std::string& path) {
    Profiles result;KeyFile key(g_key_file_new(),g_key_file_unref);GError* error=nullptr;
    if(!g_key_file_load_from_file(key.get(),path.c_str(),G_KEY_FILE_NONE,&error)) {
        if(g_error_matches(error,G_FILE_ERROR,G_FILE_ERROR_NOENT)) {g_error_free(error);return result;}
        throw std::runtime_error(path+": "+take_error(error));
    }
    if(read(key.get(),"Settings","version")!="1") throw std::runtime_error(path+": unsupported settings version (expected 1)");
    const auto selected=read(key.get(),"Settings","selected");
    if(selected.size()!=1 || selected[0]<'1' || selected[0]>'5') throw std::runtime_error("Selected profile must be 1..5");
    result.selected=selected[0]-'0';
    for(size_t i=0;i<result.profiles.size();++i) {
        const auto group="Profile "+std::to_string(i+1);auto& profile=result.profiles[i];
        profile.name=read(key.get(),group.c_str(),"name");
        for(const auto& f:setting_fields())
            if(g_key_file_has_key(key.get(),group.c_str(),f.key,nullptr))
                set_setting(profile.settings,f.key,read(key.get(),group.c_str(),f.key));
    }
    validate_profiles(result);return result;
}
void save_profiles(const Profiles& profiles,const std::string& path) {
    validate_profiles(profiles);
    // Never silently replace an invalid or future-version document.
    (void)load_profiles(path);
    KeyFile key(g_key_file_new(),g_key_file_unref);
    g_key_file_set_integer(key.get(),"Settings","version",1);
    g_key_file_set_integer(key.get(),"Settings","selected",profiles.selected);
    for(size_t i=0;i<profiles.profiles.size();++i) {
        const auto group="Profile "+std::to_string(i+1);const auto& profile=profiles.profiles[i];
        g_key_file_set_string(key.get(),group.c_str(),"name",profile.name.c_str());
        for(const auto& f:setting_fields()) g_key_file_set_string(key.get(),group.c_str(),f.key,setting_number(f.get(profile.settings)).c_str());
    }
    gchar* parent=g_path_get_dirname(path.c_str());const int result=g_mkdir_with_parents(parent,0700);g_free(parent);
    if(result) throw std::runtime_error("Cannot create settings directory: "+std::string(std::strerror(errno)));
    gsize length=0;gchar* data=g_key_file_to_data(key.get(),&length,nullptr);GError* error=nullptr;
    const gboolean saved=g_file_set_contents_full(path.c_str(),data,length,
        static_cast<GFileSetContentsFlags>(G_FILE_SET_CONTENTS_CONSISTENT|G_FILE_SET_CONTENTS_DURABLE),0600,&error);
    g_free(data);if(!saved) throw std::runtime_error(path+": "+take_error(error));
}
}
