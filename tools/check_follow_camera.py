"""Replay Penny's actual entrance and check that Follow stays usable in the lower corridor."""
from pathlib import Path
import argparse
import json
import math
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", choices=["Release", "Debug"], default="Release")
    configuration = parser.parse_args().configuration
    output_dir = ROOT / "docs/showcase/validation"
    output_dir.mkdir(parents=True, exist_ok=True)
    checks = []
    for step in (0.05, 1 / 60):
        result = subprocess.run([
            str(ROOT / f"bin/{configuration}/HauntedToyRoom.exe"), "--no-intro", "--story",
            "--gameplay-demo", "--no-raytrace", "--camera-trace", "--story-step", str(step),
            "--frames", str(math.ceil(11 / step)), "--size", "480,300",
            "--capture", str(output_dir / "follow-camera.bmp"),
        ], cwd=ROOT, capture_output=True, text=True, timeout=60)
        (output_dir / f"follow-camera-{configuration}-{step:.3f}.log").write_text(result.stdout + result.stderr, encoding="utf8", newline="\n")
        assert result.returncode == 0, result.stderr
        rows = [list(map(float, line.split()[1:])) for line in result.stdout.splitlines()
                if line.startswith("CAMERA_TRACE ")]
        inside = [r for r in rows if r[2] < -4 and 3 < r[3] < 7]
        assert len(inside) >= 8, "Replay did not reach the ground-floor corridor"
        closest = min(math.dist(r[4:7], r[7:10]) for r in inside)
        assert closest > 1.2, f"Follow collapsed into Penny after entry: reach={closest:.3f} at dt={step}"
        assert max(abs(r[-1]) for r in inside) < 0.01, "Upstairs hallway steering altered the lower-floor camera"
        assert max(r[-2] for r in inside) < 0.1, "A wall obstructed the follow view"
        stairs = [r for r in rows if 13.5 < r[1] < 16.5 and -6.5 < r[3] < -.5 and -4 < r[2] < 0]
        assert stairs, "Replay did not reach the stairs"
        stair_reach = min(math.dist(r[4:7], r[7:10]) for r in stairs)
        assert stair_reach > 1.2, "Avoidance from the door collapsed the stair follow view"
        assert max(r[-2] for r in stairs) < 0.1, "A stair wall obstructed the follow view"
        checks.append({"step": step, "closest_corridor_reach": closest,
                       "closest_stair_reach": stair_reach, "clear_sightline": True})
        print(f"PASS {configuration} entrance/stairs Follow at dt={step:.4f}: closest reach={closest:.3f}", flush=True)
    (output_dir / f"follow-camera-{configuration}.json").write_text(
        json.dumps({"configuration": configuration, "status": "passed", "checks": checks}, indent=2), encoding="utf8")


if __name__ == "__main__":
    main()
