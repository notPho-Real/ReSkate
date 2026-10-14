"""Build one offline, double-clickable KDE desktop installer from a verified CI DLL."""
import argparse
import base64
import hashlib
import io
import json
from pathlib import Path
import re
import zipfile


def build(dll_path, commit, output):
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ValueError("Expected the DLL source commit SHA")
    dll = Path(dll_path).read_bytes()
    payload = io.BytesIO()
    with zipfile.ZipFile(payload, "w", zipfile.ZIP_DEFLATED) as archive:
        def write(name, data):
            # Stable metadata lets a package-only retry verify immutable bytes.
            member = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            member.compress_type = zipfile.ZIP_DEFLATED
            member.create_system = 3
            member.external_attr = 0o100644 << 16
            archive.writestr(member, data)
        write("ReSkate.dll", dll)
        write("installer.py", Path(__file__).with_name("deck_installer.py").read_bytes())
        write("updater.py", Path(__file__).with_name("deck_updater.py").read_bytes())
        write("manifest.json", json.dumps({"commit": commit, "sha256": hashlib.sha256(dll).hexdigest()}))
    data = payload.getvalue()
    # %k is supplied as a separate argument by KDE, never interpolated into code or a shell.
    code = ("import base64,hashlib,io,pathlib,sys,urllib.parse,zipfile; "
            "p=urllib.parse.urlparse(sys.argv[1]); "
            "f=pathlib.Path(urllib.parse.unquote(p.path) if p.scheme=='file' else sys.argv[1]); "
            "s=f.read_text().split('# ARCADE_PAYLOAD\\n',1)[1]; "
            "b=base64.b64decode(''.join(x[1:] for x in s.splitlines())); "
            "assert hashlib.sha256(b).hexdigest()=='" + hashlib.sha256(data).hexdigest() + "','Damaged installer'; "
            "z=zipfile.ZipFile(io.BytesIO(b)); "
            "exec(compile(z.read('installer.py'),'installer.py','exec'),{'__name__':'__main__','BUNDLE':z})")
    # Desktop Entry has two escape passes: key-value unescaping, then Exec quoting.
    code = code.replace("\\", "\\\\\\\\").replace('"', '\\"').replace("$", "\\$").replace("`", "\\`")
    header = ('[Desktop Entry]\nType=Application\nName=Install ReSkate Arcade Mode\n'
              'Comment=Install Arcade Mode or restore stock ReSkate\nIcon=applications-games\n'
              'Terminal=false\nExec=python3 -c "' + code + '" %k\n# ARCADE_PAYLOAD\n')
    base64_data = base64.b64encode(data).decode()
    encoded = "\n".join("#" + base64_data[i:i+120] for i in range(0, len(base64_data), 120))
    Path(output).write_text(header + encoded + "\n")
    Path(output).chmod(0o755)
    channel = {"schema": 1, "commit": commit,
               "sha256": hashlib.sha256(Path(output).read_bytes()).hexdigest(),
               "url": "https://github.com/notPho-Real/ReSkate/releases/download/arcade-build-" + commit + "/Install-ReSkate-Arcade.desktop"}
    Path(output).with_name("channel.json").write_text(json.dumps(channel, indent=2) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("dll")
    parser.add_argument("commit")
    parser.add_argument("output")
    args = parser.parse_args()
    build(args.dll, args.commit, args.output)
