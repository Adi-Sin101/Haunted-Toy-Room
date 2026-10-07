"""Reproduce the showcase images and 120-second video from the built application.

Usage: python tools/make_showcase.py --capture --video
The documents are built separately by tools/build_showcase_docs.py.
"""
from pathlib import Path
import argparse
import csv
import json
import subprocess
import textwrap

from PIL import Image, ImageDraw, ImageFont
import imageio_ffmpeg

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/showcase"
FIG = OUT / "figures"
EXE = ROOT / "bin/Release/HauntedToyRoom.exe"
LOG = OUT / "validation"
FFMPEG = imageio_ffmpeg.get_ffmpeg_exe()
ROOM = ["--cam", "-6,4.8,7,1,1.8,-2"]
SHOTS = {
    "room-night": ["--no-raytrace", *ROOM],
    "live-control": ["--no-raytrace", "--story", "--select", "0", "--drive", "0.35", "--turn", "0.12", "--seek", "8", *ROOM],
    "room-day": ["--no-raytrace", "--hour", "12", *ROOM],
    "room-ray": ["--ray-scale", "1", *ROOM],
    "ray-zero": ["--ray-scale", "1", "--bounces", "0", *ROOM],
    "house": ["--intro", "--story", "--seek", "0.1", "--no-raytrace"],
    "house-ray": ["--intro", "--story", "--seek", "0.1", "--ray-scale", "1"],
    "garden": ["--intro", "--story", "--seek", "10", "--no-raytrace"],
    "stairs": ["--intro", "--story", "--seek", "26", "--no-raytrace"],
    "woody": ["--no-raytrace", "--select", "0", "--cam", "-3,1.5,3.4,-3,1.15,0.5"],
    "jessie": ["--no-raytrace", "--select", "1", "--cam", "-0.8,1.5,4.8,-0.8,1.15,2"],
    "bullseye": ["--no-raytrace", "--select", "2", "--orbit", "50,12,4.2"],
    "buzz": ["--no-raytrace", "--select", "3", "--cam", "0.4,1.5,0.5,0.4,1.15,-1.8"],
    "car": ["--no-raytrace", "--select", "4", "--orbit", "35,18,3"],
    "mounted": ["--no-raytrace", "--select", "2", "--mount", "--seek", "1.5", "--orbit", "50,12,4.8"],
    "ball": ["--no-raytrace", "--select", "5", "--orbit", "20,20,1.6"],
    "lamp": ["--no-raytrace", "--select", "6", "--orbit", "30,10,3.5"],
    "ghost": ["--haunt", "--select", "7", "--seek", "2", "--orbit", "20,10,3", "--ray-scale", "1"],
    "penny": ["--no-raytrace", "--select", "15", "--cam", "5.5,2.2,-3.6,7.5,1.8,-4.9"],
    "desk": ["--no-raytrace", "--cam", "-2.5,3.8,-3,-5,1.8,-7"],
    "bed": ["--no-raytrace", "--cam", "4.5,3.5,-2,7.2,1.1,-5.8"],
    "bookcase": ["--no-raytrace", "--cam", "-5.5,2.5,1,-8.9,1.7,-1.5"],
    "window": ["--no-raytrace", "--cam", "2.5,4,-3,2.5,4,-9"],
    "clock": ["--no-raytrace", "--cam", "-1.2,5,-6,-1.2,5.1,-9"],
    "fan": ["--no-raytrace", "--cam", "-1.5,4.7,2,-1.5,6.8,0"],
    "poster": ["--no-raytrace", "--cam", "-7,3.7,-1,-10,3.8,-1"],
    "blocks": ["--no-raytrace", "--cam", "3,2.2,6,5.3,0.8,3.8"],
    "laser": ["--no-raytrace", "--laser-demo", "--story-step", "0.05", "--frames", "45", "--cam", "2,3.8,6,5.1,1,2"],
    "no-textures": ["--no-raytrace", "--no-textures", *ROOM],
    "ambient": ["--no-raytrace", "--no-diffuse", "--no-specular", *ROOM],
    "diffuse": ["--no-raytrace", "--no-ambient", "--no-specular", *ROOM],
    "specular": ["--no-raytrace", "--no-ambient", "--no-diffuse", *ROOM],
    "directional": ["--no-raytrace", "--no-ambient", "--light-only", "0", *ROOM],
    "point": ["--no-raytrace", "--no-ambient", "--light-only", "1", *ROOM],
    "spot": ["--no-raytrace", "--no-ambient", "--light-only", "2", *ROOM],
    "wireframe": ["--no-raytrace", "--select", "3", "--cam", "0.4,1.5,0.5,0.4,1.15,-1.8", "--wireframe"],
    "normals": ["--no-raytrace", "--select", "3", "--cam", "0.4,1.5,0.5,0.4,1.15,-1.8", "--normals"],
    "story-end": ["--story", "--intro", "--seek", "300", "--no-raytrace"],
}
for i, name in enumerate(["flat", "gouraud", "phong", "blinn"]):
    SHOTS[name] = ["--no-raytrace", "--shading", str(i), "--select", "3", "--cam", "0.4,1.5,0.5,0.4,1.15,-1.8"]


def app(name, args):
    LOG.mkdir(parents=True, exist_ok=True)
    result = subprocess.run([str(EXE), *args], cwd=ROOT, capture_output=True, text=True, timeout=240)
    (LOG / (name + ".log")).write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode:
        raise RuntimeError(f"{name} failed: {result.stdout[-1500:]} {result.stderr}")
    return result.stdout


def capture():
    FIG.mkdir(parents=True, exist_ok=True)
    for name, args in SHOTS.items():
        path = FIG / (name + ".bmp")
        extra = ["--export", str(OUT / "inventory")] if name == "room-night" else []
        app(name, ["--size", "1600,900", "--hour", "0", "--no-hud", "--frames", "3", *args, *extra, "--capture", str(path)])
        with Image.open(path) as image:
            assert image.size == (1600, 900)
            assert max(image.convert("RGB").getextrema()[0]) > 30, name
            image.save(path.with_suffix(".png"))
        path.unlink()
        print("Captured", name, flush=True)
    # Built-in light-follow rules refresh enabled flags each update; validate isolation at UploadLights.
    # Texture thumbnails are faithful exports from the CPU generators (white is the utility fallback).
    with (OUT / "inventory/textures/textures.csv").open() as manifest:
        for row in csv.DictReader(manifest):
            path = OUT / "inventory/textures" / (row["name"] + ".rgba")
            image = Image.frombytes("RGBA", (int(row["width"]), int(row["height"])), path.read_bytes()).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
            image.save(path.with_suffix(".png"))
            if row["name"] == "pickets":
                checker = Image.new("RGB", image.size)
                draw = ImageDraw.Draw(checker)
                for y in range(0,image.height,16):
                    for x in range(0,image.width,16):draw.rectangle((x,y,x+15,y+15),fill=(140,150,165) if (x//16+y//16)%2 else (230,232,235))
                checker.paste(image,mask=image.getchannel("A"));checker.save(path.with_name("pickets-coverage.png"))
    summary = {"captures": len(SHOTS), "dimensions": [1600, 900], "story_completed": "MISSION The End" in (LOG / "story-end.log").read_text()}
    assert summary["story_completed"], "Story failed to reach dawn"
    (LOG / "captures.json").write_text(json.dumps(summary, indent=2))


# Each clip is a real rendered sequence. Time captions form a narration cue sheet.
CLIPS = [
    (10, "Welcome home", "Penny's route links the garden, doorway, stairs and toy room.", ["--intro", "--story", "--no-raytrace", "--seek", "0"]),
    (10, "A furnished world", "Five reusable primitives build the house, furniture and articulated toys.", ["--no-raytrace", "--haunt", *ROOM]),
    (12, "Hierarchy in motion", "Jessie attaches to Bullseye's saddle; one parent transform moves both.", ["--no-raytrace", "--story", "--seek", "4"]),
    (12, "Flight, laser and collision", "Buzz aims at the obstacle. The nearest ray hit receives an impulse.", ["--no-raytrace", "--story", "--seek", "18"]),
    (12, "Rescue and return", "The car follows waypoints. Wheel rotation uses distance divided by radius.", ["--no-raytrace", "--story", "--seek", "38"]),
    (10, "Live character takeover", "Woody follows your controls while Jessie and Buzz continue the mission; 0 releases him, N enters full manual.", ["--no-raytrace", "--story", "--select", "0", *ROOM, "--drive", "0.35", "--turn", "0.12"]),
    (8, "Directional, point and spot", "Moonlight is directional. The lamp adds a point source and a soft cone.", ["--no-raytrace", "--haunt", *ROOM]),
    (6, "Gouraud shading", "Light is evaluated at vertices; the rasterizer interpolates the result.", ["--no-raytrace", "--shading", "1", "--select", "3", "--cam", "0.4,1.5,0.5,0.4,1.15,-1.8"]),
    (6, "Phong shading", "Interpolated normals are normalized; the reflection-vector highlight is per pixel.", ["--no-raytrace", "--shading", "2", "--select", "3", "--cam", "0.4,1.5,0.5,0.4,1.15,-1.8"]),
    (8, "Procedural textures", "UVs map grain, wallpaper, fabric and lunar craters onto shared geometry.", ["--no-raytrace", "--cam", "2.5,4,-3,2.5,4,-9"]),
    (12, "Ray tracing", "A BVH finds analytic hits; shadow rays and mirror bounces reveal the polished floor.", ["--ray-scale", "1", *ROOM]),
    (6, "A quiet morning", "The lamp turns off, the toys return home and Penny falls asleep.", ["--no-raytrace", "--story", "--intro", "--seek", "155"]),
    (8, "Thank you", "Adiba Tahsin | Roll 2107031 | CSE-4102", ["--no-raytrace", "--story", "--intro", "--seek", "300"]),
]


def video():
    assert sum(clip[0] for clip in CLIPS) == 120
    video_dir = OUT / "video"
    video_dir.mkdir(parents=True, exist_ok=True)
    font = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 23)
    title_font = ImageFont.truetype("C:/Windows/Fonts/georgiab.ttf", 31)
    cues, elapsed, encoded = [], 0, []
    for i, (seconds, title, caption, args) in enumerate(CLIPS):
        clip_signature=json.dumps([seconds,title,caption,args,"live-control-v3"])
        signature=video_dir / f"clip-{i:02d}.json"
        cached=video_dir / f"clip-{i:02d}.mp4"
        if cached.exists() and signature.exists() and signature.read_text()==clip_signature:
            encoded.append(cached); cues.append({"start":elapsed,"end":elapsed+seconds,"title":title,"narration":caption}); elapsed+=seconds; continue
        raw = video_dir / f"clip-{i:02d}.rgb"
        frames, width, height, fps = seconds * 24, 1280, 720, 24
        app(f"video-{i:02d}", ["--size", "1280,720", "--hour", "0", "--no-hud", "--record-fps", "24", "--frames", str(frames), *args, "--record", str(raw)])
        assert raw.stat().st_size == frames * width * height * 3, "Incomplete recording"
        banner = Image.new("RGBA", (1280, 720), (0, 0, 0, 0))
        draw = ImageDraw.Draw(banner)
        draw.rectangle((0, 598, 1280, 720), fill=(11, 19, 38, 242))
        draw.rectangle((32, 620, 37, 692), fill=(255, 221, 103, 255))
        draw.text((55, 609), title, font=title_font, fill=(255, 221, 103))
        for line, text in enumerate(textwrap.wrap(caption, 94)):
            draw.text((55, 655 + 29 * line), text, font=font, fill=(241, 238, 233))
        banner_path = video_dir / f"caption-{i:02d}.png"
        banner.save(banner_path)
        mp4 = video_dir / f"clip-{i:02d}.mp4"
        subprocess.run([FFMPEG, "-y", "-v", "error", "-f", "rawvideo", "-pixel_format", "rgb24", "-video_size", "1280x720", "-framerate", "24", "-i", str(raw), "-i", str(banner_path), "-filter_complex", "[0:v]vflip[scene];[scene][1:v]overlay=0:0", "-c:v", "libx264", "-crf", "19", "-preset", "fast", "-pix_fmt", "yuv420p", "-frames:v", str(frames), str(mp4)], check=True)
        # Remove only this generated temporary stream, after successful encode.
        raw.unlink()
        signature.write_text(clip_signature)
        encoded.append(mp4)
        cues.append({"start": elapsed, "end": elapsed + seconds, "title": title, "narration": caption})
        elapsed += seconds
        print("Encoded", title, flush=True)
    listing = video_dir / "clips.txt"
    listing.write_text("\n".join("file '" + p.name + "'" for p in encoded))
    subprocess.run([FFMPEG, "-y", "-v", "error", "-f", "concat", "-safe", "0", "-i", str(listing), "-c", "copy", "-movflags", "+faststart", str(OUT / "Project-Demo.mp4")], check=True)
    (video_dir / "narration.json").write_text(json.dumps(cues, indent=2))
    subtitle = []
    stamp = lambda sec: f"00:{sec//60:02d}:{sec%60:02d},000"
    for i, cue in enumerate(cues, 1):
        subtitle.append(f"{i}\n{stamp(cue['start'])} --> {stamp(cue['end'])}\n{cue['title']}\n{cue['narration']}\n")
    (OUT / "Project-Demo.srt").write_text("\n".join(subtitle), encoding="utf-8")
    subprocess.run([FFMPEG, "-v", "error", "-i", str(OUT / "Project-Demo.mp4"), "-f", "null", "-"], check=True)
    (LOG / "video.json").write_text(json.dumps({"duration_seconds": elapsed, "fps": 24, "frames": 2880, "size": [1280, 720], "decode": "passed", "audio": "live presenter narration; cues supplied"}, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", action="store_true")
    parser.add_argument("--video", action="store_true")
    options = parser.parse_args()
    if options.capture:
        capture()
    if options.video:
        video()
