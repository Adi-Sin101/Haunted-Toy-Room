"""Replay each toy under live ownership and verify the remaining mission completes."""
from pathlib import Path
import json
import math
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
LOG = ROOT / "docs/showcase/validation"
NAMES = ["Woody", "Jessie", "Bullseye", "Buzz", "RC Car"]
HOMES = [(-3, 0, .5), (-.8, 0, 2), (2.2, 0, .3), (.4, 0, -1.8), (5.5, 0, -1.8)]


def capture(actor, seconds, name):
    args = [str(ROOT / "bin/Release/HauntedToyRoom.exe"), "--no-intro", "--no-raytrace", "--story",
            "--seek", str(seconds), "--frames", "2", "--size", "640,360", "--capture", str(LOG / (name + ".bmp"))]
    if actor is not None:
        args += ["--select", str(actor), "--drive", "0.25", "--turn", "0.2"]
    result = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=120)
    output = result.stdout + result.stderr
    (LOG / (name + ".log")).write_text(output, encoding="utf-8")
    assert result.returncode == 0, output
    positions = {n: tuple(map(float, xyz.split(","))) for n, xyz in re.findall(r"^(Woody|Jessie|Bullseye|Buzz|RC Car) at ([\d.,e+\-]+)$", output, re.M)}
    assert len(positions) == 5
    return output, positions


if __name__ == "__main__":
    results = []
    for actor, name in enumerate(NAMES):
        output, positions = capture(actor, 240, "live-" + str(actor))
        assert "Mission capture: The End" in output, name + " stalls mission"
        assert "Live control: " + name in output
        assert math.dist(positions[name], HOMES[actor]) > .2, name + " was reset by director"
        for other, home in zip(NAMES, HOMES):
            if other != name:
                assert math.dist(positions[other], home) < .08, (name, other, positions[other])
        results.append({"owned": name, "mission": "The End", "owned_pose_preserved": True, "other_actors_home": True})
        print("PASS live ownership:", name, flush=True)
    _, baseline = capture(None, 8, "live-baseline")
    _, takeover = capture(0, 8, "live-comparison")
    for name in ["Buzz", "Jessie", "Bullseye", "RC Car"]:
        assert math.dist(baseline[name], takeover[name]) < .08, (name, baseline[name], takeover[name])
    results.append({"comparison": "8-second replay with and without Woody takeover", "other_actors_match": True})
    (LOG / "live-control.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    print("PASS unaffected route comparison")
