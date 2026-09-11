"""Exercise the actual executable: CLI rejection, pacing and SIGTERM cleanup."""
import os
import re
import signal
import subprocess
import sys
import time

binary = sys.argv[1]
env = dict(os.environ)
env.pop('XSCREENSAVER_WINDOW', None)
for args in [
    ['--bloom-strength','nan'], ['--bloom-strength','1.1'], ['--distortion','-1'],
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
