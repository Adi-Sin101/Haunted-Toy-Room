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
BUZZ = ["--cam", "-2.0,1.5,2.4,-3.4,1.15,0.4"]
STORY = ["--story", "--gameplay-demo", "--no-intro", "--no-raytrace"]
CHEST = ["--cam", "4.0,3.0,6.6,0.5,0.9,0.9"]
BEDROOM = ["--cam", "8.5,-1.2,-2.4,3.0,-3.0,-5.3"]
ENTRANCE = ["--cam", "12.9,-1.3,0.5,12,-2.8,8.5"]
SHOTS = {
    "room-night": ["--no-raytrace", *ROOM],
    "live-control": [*STORY, "--seek", "50", "--rehearsal-stop", "--select", "0", "--drive", "0.35", "--turn", "0.12"],
    "room-day": ["--no-raytrace", "--hour", "12", *ROOM],
    "room-ray": ["--ray-scale", "1", *ROOM],
    "ray-zero": ["--ray-scale", "1", "--bounces", "0", *ROOM],
    "house": ["--intro", "--story", "--seek", "3.5", "--no-raytrace"],
    "house-ray": ["--intro", "--story", "--seek", "3.5", "--ray-scale", "1"],
    "garden": [*STORY, "--seek", "2.5"],
    "stairs": [*STORY, "--seek", "8.5", "--cam", "15.0,-1.4,-8.7,15,-2.4,-4.0"],
    "woody": ["--no-raytrace", "--select", "0", "--cam", "-3.4,1.6,5.8,-1.6,1.15,3.4"],
    "jessie": ["--no-raytrace", "--select", "1", "--cam", "0.5,2.0,6.6,0.7,1.3,3.7"],
    "bullseye": ["--no-raytrace", "--select", "2", "--cam", "5.65,2.3,-0.9,3.4,1.3,3.1"],
    "buzz": ["--no-raytrace", "--select", "3", *BUZZ],
    "car": ["--no-raytrace", "--select", "4", "--cam", "7.4,1.7,0.8,5.5,0.45,-1.8"],
    "mounted": ["--no-raytrace", "--select", "2", "--mount", "--seek", "1.5", "--cam", "5.65,3.0,-0.9,3.4,1.8,3.1"],
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
    "poster": ["--no-raytrace", "--cam", "-6.5,4.3,3.8,-10,3.8,2.2"],
    "blocks": ["--no-raytrace", "--cam", "7.2,2.8,3.6,5.3,0.8,6.9"],
    "laser": ["--no-raytrace", "--laser-demo", "--story-step", "0.05", "--frames", "45", "--cam", "7.8,3.2,2.0,4.9,1.0,4.5"],
    "no-textures": ["--no-raytrace", "--no-textures", *ROOM],
    "ambient": ["--no-raytrace", "--no-diffuse", "--no-specular", *ROOM],
    "diffuse": ["--no-raytrace", "--no-ambient", "--no-specular", *ROOM],
    "specular": ["--no-raytrace", "--no-ambient", "--no-diffuse", *ROOM],
    "directional": ["--no-raytrace", "--no-ambient", "--light-only", "0", *ROOM],
    "point": ["--no-raytrace", "--no-ambient", "--light-only", "1", *ROOM],
    "spot": ["--no-raytrace", "--no-ambient", "--light-only", "2", *ROOM],
    "wireframe": ["--no-raytrace", "--select", "3", *BUZZ, "--wireframe"],
    "normals": ["--no-raytrace", "--select", "3", *BUZZ, "--normals"],
    "story-end": [*STORY, "--seek", "90", "--cam", "6.5,-1.2,27,12,-3.2,15"],
}
SHOTS.update({
    "hallway-puzzle": [*STORY, "--seek", "12", "--cam", "16.2,2.8,6.3,12.5,0.8,3.0"],
    "train-clue": ["--no-raytrace", "--cam", "15,1.8,4.6,15.7,0.35,5.85"],
    "clock-clue": ["--no-raytrace", "--cam", "13.3,2.6,4.6,13.3,2.4,1.1"],
    "block-clue": ["--no-raytrace", "--cam", "11.8,2,4.9,11.8,0.1,2.5"],
    "keypad": ["--no-raytrace", "--cam", "12.3,1.5,4.8,12.3,1.45,6.82"],
    "chest-closed": [*STORY, "--seek", "13", "--cam", "3.0,2.1,4.7,0.5,0.9,0.9"],
    "chest-open": [*STORY, "--seek", "19.6", *CHEST],
    "toys-alive": [*STORY, "--seek", "23", *CHEST],
    "buzz-room": [*STORY, "--seek", "33", "--cam", "9.6,-1.2,-3.0,3.0,-3.0,-5.5"],
    "wardrobe": [*STORY, "--seek", "30", "--cam", "6.6,-2.0,-5.3,2.2,-2.6,-5.3"],
    # Times follow the solid-character timeline (jump ~41.8 s, laser 54.8 s, door broken 56 s, free exploration 72.7 s).
    "rescue-jump": [*STORY, "--seek", "41.8", *BEDROOM],
    "buzz-flight": [*STORY, "--seek", "44.6", "--cam", "6.6,-1.6,-7.8,3.0,-3.0,-5.0"],
    "entrance-lock": [*STORY, "--seek", "53.6", *ENTRANCE],
    "door-impact": [*STORY, "--seek", "55.6", *ENTRANCE],
    "door-debris": [*STORY, "--seek", "57.3", "--cam", "13.5,-2.4,15.0,12,-3.8,10.5"],
    "morning": [*STORY, "--seek", "70"],
})
for name,args in SHOTS.items():
    if "--story" not in args and "--intro" not in args: args.append("--manual")
for i, name in enumerate(["flat", "gouraud", "phong", "blinn"]):
    SHOTS[name] = ["--no-raytrace", "--shading", str(i), "--select", "3", *BUZZ, "--manual"]


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
    summary = {"captures": len(SHOTS), "dimensions": [1600, 900], "story_completed": "GAMEPLAY STAGE 7 / FREE EXPLORATION" in (LOG / "story-end.log").read_text()}
    assert summary["story_completed"], "Story rehearsal failed to reach free exploration"
    (LOG / "captures.json").write_text(json.dumps(summary, indent=2))


# Each clip is a real rendered sequence. Time captions form a narration cue sheet.
CLIPS = [
    (8,"Night outside the house","Penny arrives at the abandoned house. The camera cranes down from the street.",["--intro","--story","--gameplay-demo","--no-raytrace","--seek","0"]),
    (10,"In through the open door","Penny walks in; the main door closes and locks. She climbs the eighteen-step stair.",[*STORY,"--seek","3"]),
    (8,"Three clues: 257","Train 2, clock 5, blocks 7. The keypad opens the real Toy Room doors.",[*STORY,"--seek","11","--cam","13,3,4,13.5,1.5,1.8"]),
    (10,"The toy chest","The red wind-up button opens the lid. Woody, Jessie and Bullseye climb out alive.",[*STORY,"--seek","15.5",*CHEST]),
    (10,"Downstairs together","The toys follow Penny through a graph of doorway nodes and down the stairs.",[*STORY,"--seek","25"]),
    (12,"Cupboard rescue","Jessie mounts Bullseye, he jumps, she opens the wardrobe and Buzz flies out.",[*STORY,"--seek","34",*BEDROOM]),
    (12,"The locked main door","Penny tries the door. Buzz flies into position; L fires a nearest-hit laser and the door breaks.",[*STORY,"--seek","47.5",*ENTRANCE]),
    (12,"Morning escape","Everyone leaves the house. The fog thins, the sun rises and the camera pulls back.",[*STORY,"--seek","58"]),
    (8,"Free exploration","The story stays open in daylight. Adiba Tahsin / 2107031 / CSE-4102.",[*STORY,"--seek","76","--cam","6.5,-1.2,27,12,-3.2,15"]),
    (8,"Live character takeover","Woody follows your input while the others keep accompanying Penny. Zero releases ownership.",[*STORY,"--seek","50","--rehearsal-stop","--select","0","--drive","0.35","--turn","0.12","--cam","10.0,-1.4,-3.0,6.4,-3.8,-7.5"]),
    (6,"Gouraud shading","Lighting is evaluated at vertices and interpolated across triangles.",["--manual","--no-raytrace","--shading","1",*BUZZ]),
    (6,"Phong shading","Interpolated normals are normalised before per-fragment illumination.",["--manual","--no-raytrace","--shading","2",*BUZZ]),
    (10,"Analytic ray tracing","A BVH accelerates exact primitive hits, shadows, mirror paths and straight transparency.",["--manual","--ray-scale","1",*ROOM]),
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
        clip_signature=json.dumps([seconds,title,caption,args,"story-v2",executable_digest])
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
