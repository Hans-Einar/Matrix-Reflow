import os
import pathlib
import subprocess
import sys
import tempfile

exe = sys.argv[1]
with tempfile.TemporaryDirectory() as root:
    env = dict(os.environ, XDG_CONFIG_HOME=root, DISPLAY='')
    def run(*args, ok=True):
        p = subprocess.run([exe, *args, '--print-effective-settings'], env=env,
                           capture_output=True, text=True, timeout=5)
        assert (p.returncode == 0) == ok, (args, p.stdout, p.stderr)
        return dict(line.split('=', 1) for line in p.stdout.splitlines() if line and not line.startswith('#'))
    defaults = run()
    path = pathlib.Path(root) / 'matrix-reflow/settings.ini'
    path.parent.mkdir()
    text = '[Settings]\nversion=1\nselected=3\n'
    for slot in range(1, 6):
        text += f'[Profile {slot}]\nname=Slot {slot}\nspeed=0.{slot}\ncrt=1\n'
    path.write_text(text)
    assert run()['speed'] == '0.3'
    assert run('--profile', '5')['speed'] == '0.5'
    assert run('--speed', '0.8', '--profile', '2')['speed'] == '0.8'
    assert run('--profile', '2', '--speed', '0.8')['speed'] == '0.8'
    assert run('--no-config') == defaults
    assert run('--profile', '0') == defaults
    assert run('--no-crt')['crt'] == '0'
    assert run('--crt-identity', '--no-crt')['crt'] == '0'
    assert path.read_text() == text
    for args in [('--speed','nan'),('--fps-limit','2.5'),('--no-speed',),('--profile','6'),('--no-config','--profile','1')]:
        run(*args, ok=False)
    path.write_text('[Settings]\nversion=999\n')
    run(ok=False)
    assert run('--no-config') == defaults
    assert subprocess.run([exe, '--help'], env=env, capture_output=True).returncode == 0
print('profile precedence and display-independent CLI: OK')
