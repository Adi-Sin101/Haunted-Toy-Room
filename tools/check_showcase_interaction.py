"""Exercise the GLFW input callbacks through actual Windows key messages."""
import ctypes
from ctypes import wintypes
import subprocess
import time
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
LOG = ROOT / "docs/showcase/validation"
USER = ctypes.WinDLL("user32", use_last_error=True)
USER.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
USER.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
CALLBACK = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def run(name, mounted, live=False):
    args = [str(ROOT / "bin/Release/HauntedToyRoom.exe"), "--no-intro", "--no-raytrace", "--hour", "0"]
    if live:
        args += ["--story","--gameplay-demo","--seek","50","--rehearsal-stop"]
    else:
        args += ["--manual"]
    if mounted:
        args += ["--mount"]
    process = subprocess.Popen(args, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    found = []
    @CALLBACK
    def enum(hwnd, _):
        pid = wintypes.DWORD()
        USER.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value == process.pid:
            found.append(hwnd)
        return True
    try:
        for _ in range(80):
            USER.EnumWindows(enum, 0)
            if found:
                break
            time.sleep(0.05)
        assert found, "No application window"
        hwnd = found[0]
        # Window creation precedes texture/shader initialisation. Wait for the live frame title.
        title = ctypes.create_unicode_buffer(512)
        for _ in range(200):
            USER.GetWindowTextW(hwnd, title, 512)
            if "FPS" in title.value:
                break
            time.sleep(0.05)
        assert "FPS" in title.value, "Application never completed initialisation"
        time.sleep(0.3)
        def key(vk, duration=0.06):
            scan = USER.MapVirtualKeyW(vk, 0)
            USER.PostMessageW(hwnd, 0x100, vk, 1 | (scan << 16))
            time.sleep(duration)
            USER.PostMessageW(hwnd, 0x101, vk, 1 | (scan << 16) | (3 << 30))
            time.sleep(0.06)
        if live:
            key(ord("1"));key(ord("W"),0.3);key(ord("0"))
            time.sleep(4.2)
            key(ord("4"));key(ord("Q"),0.2);key(ord("L"));key(ord("0"))
            key(ord("N"));key(ord("5"));key(ord("W"),0.3);key(0x20);key(0x1B)
            output,_=process.communicate(timeout=20)
            (LOG/(name+".log")).write_text(output,encoding="utf-8")
            signals=["Live control: Woody","Live control: Buzz","Live control: released to simulation",
                     "GAMEPLAY STAGE 5 / THE LOCKED MAIN DOOR","Manual control / N resumes gameplay","RC Car moving"]
            assert process.returncode==0
            for signal in signals:assert signal in output,signal
            return {"scenario":name,"key_message_callbacks":"passed","signals":signals}
        key(ord("N")) # Retain separate full-manual regression scenarios.
        key(ord("2"))
        if mounted:
            time.sleep(0.8)
            key(ord("W"), 0.3)
            key(0x20)
            key(ord("R"))
            time.sleep(0.8)
            key(ord("W"), 0.2)
        else:
            key(ord("R"))
            key(ord("1"))
            key(ord("W"), 0.2)
            key(0x20)
        for vk in [0x71, 0x71, 0x71, 0x74, 0x75, 0x76, 0x72, ord("O"), 0x09, ord("T"), ord("L"), ord("T"), ord("U"), ord("T"), ord("L"), ord("M"), 0x08, 0x09]:
            key(vk)
        key(ord("7"))
        key(ord("R"))
        key(ord("A"), 0.2)
        key(ord("R"))
        key(ord("4"))
        key(ord("Q"), 0.2)
        key(ord("L"))
        key(ord("L"))
        key(ord("5"))
        key(ord("L"))
        key(ord("L"))
        key(0x1B)
        output, _ = process.communicate(timeout=20)
        (LOG / (name + ".log")).write_text(output, encoding="utf-8")
        assert process.returncode == 0
        expected = ["Shading: Gouraud", "Shading: Phong", "Ambient: OFF", "Diffuse: OFF", "Specular: OFF", "Textures: OFF", "mirrored", "transform reset", "Selected: Desk Lamp", "Selected: Buzz", "Selected: RC Car"]
        expected += ["Jessie dismounted.", "Bullseye moving", "Jessie moving"] if mounted else ["Jessie is too far", "Woody moving", "Woody stopped"]
        for signal in expected:
            assert signal in output, f"Missing {signal} in {name}"
        return {"scenario": name, "key_message_callbacks": "passed", "signals": expected}
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait()


if __name__ == "__main__":
    LOG.mkdir(parents=True, exist_ok=True)
    result = [run("interaction-independent", False), run("interaction-mounted", True),run("interaction-live",False,True)]
    (LOG / "interaction.json").write_text(json.dumps(result, indent=2))
    print("Passed manual, mounted and live actual-key-callback scenarios")
