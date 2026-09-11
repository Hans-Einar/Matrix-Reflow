"""Exercise the actual executable: CLI rejection, pacing and SIGTERM cleanup."""
import os
import re
import signal
import subprocess
import sys
import time
import tempfile
from pathlib import Path

binary = sys.argv[1]
env = dict(os.environ)
env.pop('XSCREENSAVER_WINDOW', None)
for args in [
    ['--snapshot-time','121'], ['--snapshot-time','nan'], ['--bloom-strength','nan'], ['--bloom-strength','1.1'], ['--distortion','-1'],
    ['--bloom-level','6'], ['--speed', 'nan'], ['--density', '0'], ['--scale', '1e300'],
    ['--depth', '-1'], ['--fps-limit', '0'], ['--fps-limit', '241'],
    ['--duration', '0'], ['--duration', 'inf'], ['--duration'],
    ['--size', '0x10'], ['--unknown'], ['--root'],
    ['--window-id', '0'], ['--windowed', '--window-id', '1'],
]:
    result = subprocess.run([binary] + args, env=env, capture_output=True, text=True, timeout=5)
    assert result.returncode == 1 and 'matrix-reflow:' in result.stderr, (args, result)

invalid_env = dict(env, XSCREENSAVER_WINDOW='invalid')
result = subprocess.run([binary], env=invalid_env, capture_output=True, text=True, timeout=5)
assert result.returncode == 1 and 'Invalid XSCREENSAVER_WINDOW' in result.stderr
# An explicit own-window request must ignore a stale host environment.
result = subprocess.run([binary, '--windowed', '--hidden', '--duration', '.25',
                         '--fps-limit', '10', '--stats'], env=invalid_env,
                        capture_output=True, text=True, timeout=5)
assert result.returncode == 0, result.stderr
match = re.search(r'completed frames=(\d+) seconds=([\d.]+)', result.stdout)
assert match and 1 <= int(match[1]) <= 3 and float(match[2]) >= .25, result.stdout
process = subprocess.Popen([binary, '--windowed', '--hidden', '--stats'], env=env,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
try:
    time.sleep(.3)
    process.send_signal(signal.SIGTERM)
    out, err = process.communicate(timeout=5)
    assert process.returncode == 0 and 'completed frames=' in out, (out, err)
finally:
    if process.poll() is None:
        process.kill()
        process.wait()
print('CLI bounds, stale host environment, FPS cap, duration and SIGTERM passed')

with tempfile.TemporaryDirectory() as directory:
    captures = []
    for frame in ('1', '3'):
        path = Path(directory) / (frame + '.ppm')
        subprocess.run([binary, '--windowed', '--hidden', '--size', '160x90',
                        '--snapshot-time', '8', '--frames', frame, '--capture', str(path)],
                       env=env, check=True, capture_output=True, timeout=5)
        captures.append(path.read_bytes())
    assert captures[0] == captures[1], 'Snapshot animation or wall clock changed between frames'
print('Fixed-time snapshots are deterministic across render frames')

with tempfile.TemporaryDirectory() as directory:
    def capture(args):
        path = Path(directory) / 'frame.ppm'
        subprocess.run([binary, '--windowed', '--hidden', '--size', '160x90',
                        '--snapshot-time', '8', '--frames', '1', '--capture', str(path)] + args,
                       env=env, check=True, capture_output=True, timeout=5)
        return path.read_bytes()
    off = capture(['--no-crt'])
    on = capture(['--crt'])
    assert off != on, 'CRT has no effect'
    assert on == capture(['--crt-identity', '--crt']), 'Explicit CRT did not override identity'
    identity = capture(['--crt-identity'])
    assert max(abs(a-b) for a,b in zip(off, identity)) <= 1, 'Identity output mismatch'
    assert off == capture(['--crt', '--no-crt']), 'CRT bypass changed output'
    assert capture(['--no-post']) == capture(['--crt', '--no-post']), 'Raw diagnostics did not bypass CRT'
print('CRT flags, identity, override order and raw bypass passed')
