"""Replay the complete story (outside -> 257 -> chest -> wardrobe -> laser -> morning) in every shading path and Debug."""
from pathlib import Path
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/showcase/validation"


def main():
    results = []
    for mode in ["flat", "gouraud", "phong", "blinn", "ray", "debug"]:
        executable = ROOT / ("bin/Debug/HauntedToyRoom.exe" if mode == "debug" else "bin/Release/HauntedToyRoom.exe")
        args = [str(executable), "--no-intro", "--story", "--gameplay-demo", "--seek", "95",
                "--frames", "3", "--size", "960,600", "--capture", str(OUT / ("escape-" + mode + ".bmp"))]
        if mode != "ray":
            args += ["--no-raytrace"]
        if mode in ["flat", "gouraud", "phong", "blinn"]:
            args += ["--shading", str(["flat", "gouraud", "phong", "blinn"].index(mode))]
        process = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=180)
        output = process.stdout + process.stderr
        (OUT / ("escape-" + mode + ".log")).write_text(output, encoding="utf8")
        assert process.returncode == 0, (mode, process.returncode, output[-1500:])
        assert "GAMEPLAY STAGE 7 / FREE EXPLORATION" in output and "ESCAPE real laser broke entrance door" in output, mode
        actual = re.findall(r"^(Penny|Woody|Jessie|Bullseye|Buzz) at ([\d.,e+\-]+)", output, re.M)
        assert len(actual) == 5
        for name, xyz in actual:
            x, y, z = map(float, xyz.split(","))
            assert y < -.3 and z > 13, (mode, name, xyz)
        results.append({"path": mode, "real_door_impact": True, "free_exploration": True,
                        "all_five_actual_positions_outside": True, "process_exit": 0})
        print("PASS full escape:", mode, flush=True)
    (OUT / "escape-rendering.json").write_text(json.dumps(results, indent=2), encoding="utf8")


if __name__ == "__main__":
    main()
