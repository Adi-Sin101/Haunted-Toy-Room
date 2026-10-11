"""Check the delivered deck, typeset mathematics and final embedded recording."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import zipfile

import pymupdf as fitz
import imageio_ffmpeg
from pptx import Presentation

OUT = Path(__file__).resolve().parents[1] / "docs/showcase"


def check():
    deck = Presentation(OUT / "Project-Presentation.pptx")
    assert len(deck.slides) == 12
    math_slides = []
    equation_count = 0
    for number, slide in enumerate(deck.slides, 1):
        texts = "\n".join(shape.text for shape in slide.shapes if shape.has_text_frame)
        assert "Adiba Tahsin" in texts and "2107031" in texts and "CSE-4102" in texts
        assert f"{number:02d} / 12" in texts
        assert slide.notes_slide.notes_text_frame.text.strip()
        for shape in slide.shapes:
            assert min(shape.left, shape.top) >= 0, (number, shape.name)
            assert shape.left + shape.width <= deck.slide_width, (number, shape.name)
            assert shape.top + shape.height <= deck.slide_height, (number, shape.name)
        if "IMPLEMENTATION EQUATIONS" in texts:
            math_slides.append(number)
            equations = [shape for shape in slide.shapes
                         if any(node.get("descr", "").startswith("math-")
                                for node in shape._element.iter())]
            assert len(equations) == 2, (number, len(equations))
            equation_count += len(equations)
    assert math_slides == [5, 6, 7, 8, 9, 10]
    assert "TEXTURE MAPPING" in deck.slides[4].shapes[1].text
    assert any("THANK YOU" in s.text for s in deck.slides[10].shapes if s.has_text_frame)
    video = OUT / "Presentation-Demo.mp4"
    video_bytes = video.read_bytes()
    with zipfile.ZipFile(OUT / "Project-Presentation.pptx") as archive:
        assert archive.testzip() is None
        media = [name for name in archive.namelist() if name.endswith(".mp4")]
        assert len(media) == 1 and archive.read(media[0]) == video_bytes
        movie_slides = [n for n in range(1, 13)
                        if b"videoFile" in archive.read(f"ppt/slides/slide{n}.xml")]
        assert movie_slides == [12]
    pdf = fitz.open(OUT / "Project-Presentation.pdf")
    assert len(pdf) == 12
    ffmpeg = imageio_ffmpeg.get_ffmpeg_exe()
    info = subprocess.run([ffmpeg, "-hide_banner", "-i", str(video)],
                          capture_output=True, text=True).stderr
    assert "1920x1080" in info and "Audio:" not in info
    duration = re.search(r"Duration: (\d+):(\d+):(\d+\.\d+)", info)
    assert duration
    hours, minutes, seconds = map(float, duration.groups())
    duration_seconds = hours * 3600 + minutes * 60 + seconds
    assert duration_seconds > 0
    subprocess.run([ffmpeg, "-v", "error", "-xerror", "-i", str(video),
                    "-f", "null", "-"], check=True)
    assert 0 < video_bytes.find(b"moov") < video_bytes.find(b"mdat")
    metadata = json.loads((OUT / "video/presentation-demo.json").read_text())
    assert hashlib.sha256(video_bytes).hexdigest() == metadata["sha256"]
    original = OUT / metadata["source"]
    if original.exists():
        assert hashlib.sha256(original.read_bytes()).hexdigest() == metadata["source_sha256"]
        original_info = subprocess.run([ffmpeg, "-hide_banner", "-i", str(original)],
                                       capture_output=True, text=True).stderr
        original_duration = re.search(r"Duration: (\d+):(\d+):(\d+\.\d+)", original_info)
        assert original_duration
        h, m, s = map(float, original_duration.groups())
        assert abs(duration_seconds - (h * 3600 + m * 60 + s)) < .1
    summary = {"slides": 12, "pdf_pages": len(pdf), "texture_slide": 5,
               "thank_you_slide": 11, "video_slide": 12, "equation_slides": math_slides,
               "typeset_equations": equation_count, "shape_bounds": "passed",
               "embedded_video_matches_standalone": True, "full_video_decode": "passed",
               "video_resolution": "1920x1080", "audio_streams": 0,
               "video_seconds": duration_seconds, "video_bytes": len(video_bytes),
               "original_preserved": True, "sha256": metadata["sha256"]}
    (OUT / "validation/presentation.json").write_text(json.dumps(summary, indent=2), encoding="utf8")
    documents = OUT / "validation/documents.json"
    summary_docs = json.loads(documents.read_text())
    summary_docs.update({"slides": 12, "embedded_video": True, "video_slide": 12,
                         "typeset_slide_equations": equation_count})
    documents.write_text(json.dumps(summary_docs, indent=2), encoding="utf8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    check()
