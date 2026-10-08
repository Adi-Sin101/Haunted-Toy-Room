"""Reproduce the showcase images and 120-second video from the built application.

Usage: python tools/make_showcase.py --capture --video
The documents are built separately by tools/build_showcase_docs.py.
"""
from pathlib import Path
import argparse
import csv
import hashlib
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
    "live-control": ["--no-raytrace", "--story", "--gameplay-demo", "--seek", "30", "--rehearsal-stop", "--select", "0", "--drive", "0.35", "--turn", "0.12", *ROOM],
    "room-day": ["--no-raytrace", "--hour", "12", *ROOM],
    "room-ray": ["--ray-scale", "1", *ROOM],
    "ray-zero": ["--ray-scale", "1", "--bounces", "0", *ROOM],
    "house": ["--intro", "--story", "--seek", "0.1", "--no-raytrace"],
    "house-ray": ["--intro", "--story", "--seek", "0.1", "--ray-scale", "1"],
    "garden": ["--intro", "--story", "--seek", "10", "--no-raytrace"],
    "stairs": ["--intro", "--story", "--seek", "26", "--no-raytrace"],
    "woody": ["--no-raytrace", "--select", "0", "--cam", "-3,1.5,3.4,-3,1.15,0.5"],
    "jessie": ["--no-raytrace", "--select", "1", "--cam", "-0.8,1.5,4.8,-0.8,1.15,2"],
    "bullseye": ["--no-raytrace", "--select", "2", "--cam", "2.5,2.8,4.6,2.2,1.35,0.3"],
    "buzz": ["--no-raytrace", "--select", "3", "--cam", "0,1.5,-4.5,0,1.15,-6.6"],
    "car": ["--no-raytrace", "--select", "4", "--cam", "7.4,1.7,0.8,5.5,0.45,-1.8"],
    "mounted": ["--no-raytrace", "--select", "2", "--mount", "--seek", "1.5", "--cam", "2.5,3.7,5.5,2.2,2,0.3"],
    "ball": ["--no-raytrace", "--select", "5", "--orbit", "20,20,1.6"],
    "lamp": ["--no-raytrace", "--select", "6", "--orbit", "30,10,3.5"],
    "ghost": ["--haunt", "--select", "7", "--seek", "2", "--orbit", "20,10,3", "--ray-scale", "1"],
    "penny": ["--no-raytrace", "--select", "15", "--cam", "12.1,1.25,6.8,13.3,0.65,4.5"],
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
    "wireframe": ["--no-raytrace", "--select", "3", "--cam", "0,1.5,-4.5,0,1.15,-6.6", "--wireframe"],
    "normals": ["--no-raytrace", "--select", "3", "--cam", "0,1.5,-4.5,0,1.15,-6.6", "--normals"],
    "story-end": ["--story", "--gameplay-demo", "--no-intro", "--seek", "75", "--no-raytrace"],
}
SHOTS.update({
    "hallway-puzzle":["--story","--gameplay-demo","--seek","0.1","--no-raytrace","--cam","13,3,4,13.5,1.5,1.8"],
    "train-clue":["--no-raytrace","--cam","14.4,2.2,7.2,15.3,0.35,5.5"],
    "clock-clue":["--no-raytrace","--cam","13.3,2.5,4.8,13.3,2.4,1.2"],
    "block-clue":["--no-raytrace","--cam","11.8,2,4.9,11.8,0.1,2.5"],
    "keypad":["--no-raytrace","--cam","12.3,1.5,4.8,12.3,1.45,6.82"],
    "rescue-switches":["--story","--gameplay-demo","--seek","8","--no-raytrace","--cam","6,3,7,4,1.4,3.5"],
    "high-switch":["--story","--gameplay-demo","--seek","13.8","--no-raytrace","--cam","0.5,3.5,7,2.5,2.2,4.5"],
    "buzz-barrier":["--story","--gameplay-demo","--seek","14.8","--no-raytrace","--cam","0,2.3,-1,0,1.5,-5"],
    "ghost-chase":["--story","--gameplay-demo","--seek","20","--no-raytrace",*ROOM],
    "stair-descent":["--story","--gameplay-demo","--seek","23.5","--no-raytrace","--cam","16,-1,-5,15,-2,-3"],
    "entrance-lock":["--story","--gameplay-demo","--seek","35","--no-raytrace","--cam","12,-2.8,6.6,12,-3.1,9.2"],
    "door-impact":["--story","--gameplay-demo","--seek","51.9","--no-raytrace","--cam","10,-1.8,6,12,-2.9,9.2"],
    "door-debris":["--story","--gameplay-demo","--seek","53.5","--no-raytrace","--cam","9,-1,13,12,-3.4,9.2"],
})
for name,args in SHOTS.items():
    if "--story" not in args and "--intro" not in args: args.append("--manual")
for i, name in enumerate(["flat", "gouraud", "phong", "blinn"]):
    SHOTS[name] = ["--no-raytrace", "--shading", str(i), "--select", "3", "--cam", "0,1.5,-4.5,0,1.15,-6.6"]


def app(name, args):
    LOG.mkdir(parents=True, exist_ok=True)
    result = subprocess.run([str(EXE), *args], cwd=ROOT, capture_output=True, text=True, timeout=240)
    (LOG / (name + ".log")).write_text((result.stdout + result.stderr).rstrip()+"\n", encoding="utf-8")
    if result.returncode:
        raise RuntimeError(f"{name} failed: {result.stdout[-1500:]} {result.stderr}")
    return result.stdout


def capture():
    FIG.mkdir(parents=True, exist_ok=True)
    for name, args in SHOTS.items():
        path = FIG / (name + ".bmp")
        extra = ["--export", str(OUT / "inventory")] if name == "room-night" else []
        app(name, ["--size", "1600,900", "--hour", "20.5", "--no-hud", "--frames", "3", *args, *extra, "--capture", str(path)])
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
    summary = {"captures": len(SHOTS), "dimensions": [1600, 900], "story_completed": "GAMEPLAY WIN / ENDING" in (LOG / "story-end.log").read_text()}
    assert summary["story_completed"], "Escape rehearsal failed to reach WIN"
    (LOG / "captures.json").write_text(json.dumps(summary, indent=2))


# Each clip is a real rendered sequence. Time captions form a narration cue sheet.
CLIPS = [
    (10,"The haunted house","Penny enters at night through hinged doors and the connected staircase.",["--intro","--story","--no-raytrace","--seek","0"]),
    (10,"A furnished world","Five indexed primitive families form furniture and articulated toys.",["--manual","--no-raytrace",*ROOM]),
    (10,"Inspect the three clues","Train 2, clock 5, blocks 7. The code opens the real toy-room doors.",["--story","--gameplay-demo","--seek","0","--no-raytrace"]),
    (12,"Rescue through hierarchy","Penny activates two switches. Jessie rides Bullseye and dismounts onto the high platform.",["--story","--gameplay-demo","--seek","7","--no-raytrace","--cam","1,4,7,3,1.5,3"]),
    (10,"Buzz and the pursuit","The release removes a translucent barrier. The ghost follows Penny to the stairs.",["--story","--gameplay-demo","--seek","15","--no-raytrace"]),
    (10,"Live character takeover","Woody follows your input while the others continue. Zero releases ownership; N enters full manual mode.",["--story","--gameplay-demo","--seek","30","--rehearsal-stop","--select","0","--drive","0.35","--turn","0.12","--no-raytrace",*ROOM]),
    (8,"Directional, point and spot","Moonlight is directional; the lamp adds a point source and soft spotlight.",["--manual","--haunt","--no-raytrace",*ROOM]),
    (6,"Gouraud shading","Lighting is evaluated at vertices and interpolated across triangles.",["--manual","--no-raytrace","--shading","1","--cam","0,1.5,-4.5,0,1.15,-6.6"]),
    (6,"Phong shading","Interpolated normals are normalised before per-fragment illumination.",["--manual","--no-raytrace","--shading","2","--cam","0,1.5,-4.5,0,1.15,-6.6"]),
    (8,"Texture coordinates","UVs map wood, wallpaper, fabric and lunar craters onto shared geometry.",["--manual","--no-raytrace","--cam","2.5,4,-3,2.5,4,-9"]),
    (10,"Analytic ray tracing","A BVH accelerates exact primitive hits, shadows, mirror paths and straight transparency.",["--manual","--ray-scale","1",*ROOM]),
    (12,"The entrance breaks","Buzz flies and aims. Only a nearest-hit laser impact releases six physical wood fragments.",["--story","--gameplay-demo","--seek","45","--no-raytrace","--cam","10,-1.8,6,12,-2.9,9.2"]),
    (8,"The toys are safe","Every character is physically outside. The cast idles in the garden. Adiba Tahsin / 2107031 / CSE-4102.",["--story","--gameplay-demo","--seek","75","--no-raytrace"]),
]


def video():
    assert sum(clip[0] for clip in CLIPS) == 120
    video_dir = OUT / "video"
    video_dir.mkdir(parents=True, exist_ok=True)
    font = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 23)
    title_font = ImageFont.truetype("C:/Windows/Fonts/georgiab.ttf", 31)
    cues, elapsed, encoded = [], 0, []
    executable_digest=hashlib.sha256(EXE.read_bytes()).hexdigest()
    for i, (seconds, title, caption, args) in enumerate(CLIPS):
        clip_signature=json.dumps([seconds,title,caption,args,"escape-v1",executable_digest])
        signature=video_dir / f"clip-{i:02d}.json"
        cached=video_dir / f"clip-{i:02d}.mp4"
        if cached.exists() and signature.exists() and signature.read_text()==clip_signature:
            encoded.append(cached); cues.append({"start":elapsed,"end":elapsed+seconds,"title":title,"narration":caption}); elapsed+=seconds; continue
        raw = video_dir / f"clip-{i:02d}.rgb"
        frames, width, height, fps = seconds * 24, 1280, 720, 24
        app(f"video-{i:02d}", ["--size", "1280,720", "--hour", "20.5", "--no-hud", "--record-fps", "24", "--frames", str(frames), *args, "--record", str(raw)])
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
