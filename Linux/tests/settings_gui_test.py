"""Optional short AT-SPI integration check on a private Xvfb.
Run with /usr/bin/python3 tests/settings_gui_test.py BUILD_DIR OUTPUT_DIR.
Requires PyGObject/AT-SPI, an accessible session bus, Xvfb and GLX software rendering.
Never changes live settings, moves the pointer, or activates a screen lock.
"""
import configparser
import os
from pathlib import Path
import signal
import subprocess as sp
import sys
import tempfile
import time
import gi

if len(sys.argv)>1 and sys.argv[1]=='--capture':
    gi.require_version('Gdk','3.0')
    from gi.repository import Gdk
    display=Gdk.Display.open(sys.argv[2]); root=display.get_default_screen().get_root_window()
    pix=Gdk.pixbuf_get_from_window(root,0,0,root.get_width(),root.get_height())
    assert pix;pix.savev(sys.argv[3],'png',[],[])
    sys.exit(0)

gi.require_version('Atspi','2.0')
from gi.repository import Atspi
build=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=True)

def walk(a):
    yield a
    for i in range(a.get_child_count()):
        try: yield from walk(a.get_child_at_index(i))
        except Exception: pass

def until(fn, seconds=5):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        value=fn()
        if value:return value
        time.sleep(.05)
    raise AssertionError('Timed out: '+str(fn))

def app_for(pid):
    return next((a for a in walk(Atspi.get_desktop(0)) if a.get_role_name()=='application' and a.get_process_id()==pid),None)

def child_commands(pid):
    result={}
    for p in Path('/proc').glob('[0-9]*'):
        try:
            if (p/'stat').read_text().split(') ',1)[1].split()[1]==str(pid):
                argv=(p/'cmdline').read_bytes().split(b'\0')
                if argv[0].endswith(b'/matrix-reflow'):result[int(p.name)]=[v.decode() for v in argv if v]
        except OSError:pass
    return result

r,w=os.pipe()
x=sp.Popen(['Xvfb','-displayfd',str(w),'-screen','0','1280x800x24','+extension','GLX','-nolisten','tcp'],pass_fds=(w,),stderr=sp.DEVNULL)
os.close(w);display=':'+os.read(r,32).decode().strip();os.close(r)
s=None;unrelated=None
try:
    with tempfile.TemporaryDirectory() as temp:
        env=dict(os.environ,DISPLAY=display,XDG_CONFIG_HOME=temp,LIBGL_ALWAYS_SOFTWARE='1')
        env.pop('XSCREENSAVER_WINDOW',None)
        path=Path(temp)/'matrix-reflow/settings.ini'
        def launch():
            global s
            s=sp.Popen([str(build/'matrix-reflow-settings')],env=env,stdout=open(out/'settings.log','a'),stderr=sp.STDOUT)
            return until(lambda:app_for(s.pid))
        app=launch()
        def find(role,name=None):
            return next(a for a in walk(app) if a.get_role_name()==role and (name is None or a.get_name()==name))
        def action(role,name):
            assert find(role,name).get_action_iface().do_action(0)
            time.sleep(.1)
        def tab(name):
            tabs=find('page tab list');index=['Rain','Scene','Effects','Colors'].index(name)
            assert tabs.get_selection_iface().select_child(index);time.sleep(.1)
        def select(slot):
            combo=find('combo box');assert combo.get_selection_iface().select_child(slot-1);time.sleep(.1)
        def numeric(name,value):
            assert find('spin button',name).get_value_iface().set_current_value(value);time.sleep(.05)
        def effective(args):
            p=sp.run([str(build/'matrix-reflow'),*args,'--print-effective-settings'],env=env,capture_output=True,text=True,check=True)
            return dict(l.split('=',1) for l in p.stdout.splitlines() if l and not l.startswith('#'))
        for slot in range(1,6):
            select(slot)
            assert find('text').get_editable_text_iface().set_text_contents(f'Test {slot}')
            numeric('Speed',slot/10)
            tab('Effects');numeric('Frame limit',15);tab('Rain')
        select(3);numeric('Speed',.73)
        tab('Colors');action('button','Main rain color')
        # GTK standard color editor accepts a custom CSS hex color.
        texts=[a for a in walk(app) if a.get_role_name()=='text' and a.get_name().lower()=='color name']
        assert texts
        texts[0].get_component_iface().grab_focus()
        assert texts[0].get_editable_text_iface().set_text_contents('#33CC66')
        assert texts[0].get_action_iface().do_action(0)
        find('button','Select').get_component_iface().grab_focus()
        time.sleep(.2)
        sp.run([sys.executable,__file__,'--capture',display,str(out/'color-dialog.png')],check=True)
        action('button','Select')
        action('button','Preview')
        children=until(lambda:child_commands(s.pid));assert len(children)==1
        first_pid,args=next(iter(children.items()))
        values=effective(args[1:]);assert values['speed']=='0.73'
        assert abs(float(values['main-red'])-.2)<1e-6, values
        assert not path.exists(),'Preview wrote profile'
        # Save uses precisely the unsaved preview snapshot, including float colors.
        action('button','Save profiles');assert path.exists()
        assert effective(['--profile','3'])==values
        cfg=configparser.ConfigParser();cfg.read(path)
        assert cfg['Settings']['selected']=='3'
        for slot in range(1,6):assert cfg[f'Profile {slot}']['name']==f'Test {slot}'
        action('button','Stop preview');until(lambda:not Path(f'/proc/{first_pid}').exists())
        sp.run([sys.executable,__file__,'--capture',display,str(out/'settings-colors.png')],check=True)
        tab('Rain');sp.run([sys.executable,__file__,'--capture',display,str(out/'settings-rain.png')],check=True)
        # Restart must restore all five named slots and active profile values.
        s.terminate();s.wait(timeout=4);app=launch()
        assert find('combo box').get_name()=='3 · Test 3'
        assert abs(find('spin button','Speed').get_value_iface().get_current_value()-.73)<1e-6
        unrelated=sp.Popen([str(build/'matrix-reflow'),'--no-config','--windowed','--hidden','--fps-limit','10','--size','160x90'],env=env,stdout=sp.DEVNULL,stderr=sp.DEVNULL)
        action('button','Preview');children=until(lambda:child_commands(s.pid));second_pid=next(iter(children))
        time.sleep(2)
        sp.run([sys.executable,__file__,'--capture',display,str(out/'preview.png')],check=True)
        s.terminate();s.wait(timeout=4)
        until(lambda:not Path(f'/proc/{second_pid}').exists())
        assert unrelated.poll() is None,'Unrelated renderer was stopped'
        assert not child_commands(s.pid)
        print('Five profiles, color editing, unsaved argv parity, restart, own-preview cleanup: OK',flush=True)
finally:
    if s and s.poll() is None:s.terminate();s.wait(timeout=4)
    if unrelated and unrelated.poll() is None:unrelated.terminate();unrelated.wait(timeout=4)
    x.terminate();x.wait(timeout=3)
