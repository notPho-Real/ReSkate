"""Explicit, interactive administrator action authorized for the paired Deck."""
import shlex
import subprocess
import controller

# Password is read by remote sudo from its terminal, never by Python.
# No reboot, network reconnect, suspend, service install or game-file mutation.
REMOTE = r'''
import os, subprocess
from pathlib import Path
if os.geteuid() != 0:
    raise SystemExit("Administrator authentication required")
def call(*args):
    return subprocess.check_output(args, text=True).strip()
device = Path('/sys/class/net/wlan0/device')
if (device / 'driver').resolve().name != 'rtw88_8822ce':
    raise SystemExit('Unexpected Wi-Fi driver; no changes made')
profile = call('nmcli', '-g', 'GENERAL.CON-UUID', 'device', 'show', 'wlan0')
import re
if not re.fullmatch(r'[0-9a-f-]{36}', profile):
    raise SystemExit('No unambiguous active Wi-Fi profile')
setting = '802-11-wireless.wake-on-wlan'
before = call('nmcli', '-g', setting, 'connection', 'show', 'uuid', profile)
wake = device / 'power/wakeup'
power_before = wake.read_text().strip()
driver_before = call('iw', 'phy', 'phy0', 'wowlan', 'show')
if before != '0x1' or power_before != 'disabled' or driver_before != 'WoWLAN is disabled.':
    raise SystemExit('Settings differ from inspected baseline; no changes made')
try:
    call('nmcli', 'connection', 'modify', 'uuid', profile, setting, 'magic')
    call('iw', 'phy', 'phy0', 'wowlan', 'enable', 'magic-packet')
    wake.write_text('enabled\n')
    if call('nmcli', '-g', setting, 'connection', 'show', 'uuid', profile) != '0x8':
        raise RuntimeError('Profile verification failed')
    if 'magic packet' not in call('iw', 'phy', 'phy0', 'wowlan', 'show') or wake.read_text().strip() != 'enabled':
        raise RuntimeError('Device verification failed')
except BaseException:
    failures = []
    for action in (lambda: call('nmcli', 'connection', 'modify', 'uuid', profile, setting, 'default'),
                   lambda: call('iw', 'phy', 'phy0', 'wowlan', 'disable'),
                   lambda: wake.write_text('disabled\n')):
        try: action()
        except BaseException: failures.append(True)
    print('Rollback incomplete; inspect settings' if failures else 'Original wake settings restored')
    raise
print('WIFI WAKE ENABLED. Actual sleep/wake and reboot persistence still need testing.')
'''

if __name__ == '__main__':
    command = 'sudo -- python3 -c ' + shlex.quote(REMOTE)
    args = controller.ssh_arguments('deck@steamdeck.local', controller.HOME)
    args.insert(1, '-tt')
    subprocess.run(args + [command], check=True)
