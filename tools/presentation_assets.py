"""Typeset slide mathematics and prepare an embedded, portable demo recording."""
from pathlib import Path
import hashlib
import json
import subprocess

import imageio_ffmpeg
from PIL import Image, ImageDraw
from matplotlib.figure import Figure
from matplotlib.backends.backend_agg import FigureCanvasAgg


EQUATIONS = {
    "TRANSFORMS + INTERACTION": [
        r"$\mathbf{M}_{w}=\mathbf{M}_{parent}\,\mathbf{M}_{local},\qquad \mathbf{M}_{local}=\mathbf{T}\mathbf{R}\mathbf{S}$",
        r"$\mathbf{n}_{w}=\mathrm{normalize}\!\left((\mathbf{A}^{-1})^{T}\mathbf{n}_{o}\right)$",
    ],
    "ANIMATION + CONTACT": [
        r"$\mathbf{p}_{t+\Delta t}=\mathbf{p}_{t}+\mathbf{v}\Delta t,\qquad \Delta\theta=\frac{s}{r}$",
        r"$a=1-e^{-k\Delta t},\qquad \mathbf{x}_{new}=(1-a)\mathbf{x}_{old}+a\mathbf{x}_{target}$",
    ],
    "LIGHTING": [
        r"$\mathbf{C}=\mathbf{A}\odot k_a\mathbf{I}_a+\sum_i v_i a_i\mathbf{I}_i\odot\left(\mathbf{A}k_d[\mathbf{N}\cdot\mathbf{L}_i]_++k_s[\mathbf{R}_i\cdot\mathbf{V}]_+^{n}\right)+\mathbf{E}$",
        r"$a(d)=\frac{1}{k_c+k_l d+k_q d^2},\qquad S=\mathrm{smoothstep}(\cos\theta_{out},\cos\theta_{in},-\mathbf{L}\cdot\mathbf{D})$",
    ],
    "SHADING": [
        r"$\mathbf{C}_{G}=\sum_i\widehat{\lambda}_i\mathbf{C}_i,\qquad \mathbf{N}_{P}=\mathrm{normalize}\!\left(\sum_i\widehat{\lambda}_i\mathbf{N}_i\right),\quad i\in\{0,1,2\}$",
        r"$\mathbf{H}=\mathrm{normalize}(\mathbf{L}+\mathbf{V}),\qquad I_{s,Blinn}=k_s[\mathbf{N}\cdot\mathbf{H}]_+^{4n}$",
    ],
    "RAY TRACING + SHADOWS": [
        r"$\mathbf{r}(t)=\mathbf{o}+t\mathbf{d},\qquad \mathbf{d}_{reflect}=\mathbf{d}-2(\mathbf{d}\cdot\mathbf{N})\mathbf{N}$",
        r"$\mathbf{C}_{acc}\leftarrow\mathbf{C}_{acc}+T w\mathbf{C}_{local},\qquad T\leftarrow T\rho,\quad T_0=1,\quad b=2$",
    ],
    "TEXTURE MAPPING": [
        r"$(u',v')=(s_u u,s_v v),\qquad \mathbf{A}=\mathbf{k}_{RGB}\odot\mathrm{texture}(u',v',\ell)_{RGB}$",
        r"$\alpha_{tex}<0.5\ \Longrightarrow\ \mathrm{discard},\qquad N_{layers}=27+1=28$",
    ],
}

NOTES = {
    "TRANSFORMS + INTERACTION": "A is the upper-left 3 by 3 linear part of the world matrix. The inverse transpose preserves normal directions under nonuniform scale and shear. T, R and S are local translation, rotation and scaling; additional basis edits carry shear or reflection.",
    "ANIMATION + CONTACT": "s is travelled arc length and r is wheel/ball radius; theta is in radians. Exponential easing gives the same response over equal elapsed time at different frame rates. k controls the response rate.",
    "LIGHTING": "All RGB products are componentwise. A is textured albedo; Ia ambient radiance; Ii light colour times intensity; v visibility; ai attenuation including a spotlight's cone factor; E emission. [x]+ means max(x,0). N,L,V,R are unit vectors. Specular contributes only on the lit side (N dot L > 0). Directional attenuation is 1. Point attenuation uses distance d; the spot multiplies it by S, with inner and outer cosine thresholds.",
    "SHADING": "The lambda-hat values are perspective-correct interpolation weights. Gouraud interpolates vertex lighting; Phong interpolates normals then evaluates lighting per fragment. The project uses shininess times four for Blinn to roughly match the Phong highlight width.",
    "RAY TRACING + SHADOWS": "Cacc starts at zero; each surface adds T times w times its locally lit colour, then updates the scalar path throughput T. At an opaque reflective hit rho is reflectivity and w=1-rho. At a transparent hit rho=1-opacity, w=opacity and the direction continues straight. At the final permitted hit w=1 closes the finite path. Default b=2 means one primary plus at most two continuations (three surface evaluations); shadow rays do not consume colour bounces. Sky contributions and early termination are handled separately. There is no physical refraction or diffuse indirect light.",
    "TEXTURE MAPPING": "u,v are primitive UVs; su,sv are material repeats; ell is the surface-array layer. RGB is multiplied by the base material colour before illumination. 27 mapped layers share a 512 by 512 array with one white fallback. Mipmaps and filtering reduce minification aliasing. The alpha cutoff applies to cutout fences, not the separate material-opacity blending used for glass and ghosts.",
}


def math_rows(output):
    output.mkdir(parents=True, exist_ok=True)
    paths = {}
    for tag, expressions in EQUATIONS.items():
        paths[tag] = []
        for index, expression in enumerate(expressions):
            figure = Figure(figsize=(28, 1.8), dpi=200)
            FigureCanvasAgg(figure)
            figure.text(.01, .4, expression, fontsize=36, color="#f6f1e2", va="center")
            path = output / f"math-{tag.lower().replace(' ', '-').replace('+', 'and')}-{index}.png"
            figure.savefig(path, transparent=True, bbox_inches="tight", pad_inches=.04)
            paths[tag].append(path)
    return paths


def movie_assets(showcase):
    source = showcase / "Haunted-Toy-Room-demo.mp4"
    destination = showcase / "Presentation-Demo.mp4"
    cache = showcase / "video/presentation-demo.json"
    cache.parent.mkdir(parents=True, exist_ok=True)
    settings = "h264-crf21-original-resolution-silent-v3"
    prior = json.loads(cache.read_text()) if cache.exists() else {}
    # A checkout contains the compact recording; retain it when the large original is absent.
    reuse = (not source.exists() and destination.exists() and
             prior.get("source") == source.name and prior.get("settings") == settings and
             prior.get("sha256") == hashlib.sha256(destination.read_bytes()).hexdigest())
    if not source.exists() and not reuse:
        source = showcase / "Project-Demo.mp4"
    fingerprint = prior["source_sha256"] if reuse else hashlib.sha256(source.read_bytes()).hexdigest()
    ffmpeg = imageio_ffmpeg.get_ffmpeg_exe()
    if not reuse and (prior.get("source_sha256") != fingerprint or prior.get("settings") != settings or not destination.exists()):
        subprocess.run([ffmpeg, "-y", "-v", "error", "-i", str(source), "-map", "0:v:0", "-an",
                        "-c:v", "libx264", "-preset", "medium", "-crf", "21", "-pix_fmt", "yuv420p",
                        "-movflags", "+faststart", str(destination)], check=True)
    poster = showcase / "slides/video-poster.png"
    poster.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([ffmpeg, "-y", "-v", "error", "-ss", "4", "-i", str(destination),
                    "-frames:v", "1", str(poster)], check=True)
    image = Image.open(poster).convert("RGB")
    draw = ImageDraw.Draw(image)
    x, y = image.width // 2, image.height // 2
    draw.ellipse((x-48, y-48, x+48, y+48), fill="#0f192f", outline="#ffdb67", width=4)
    draw.polygon([(x-13, y-25), (x-13, y+25), (x+27, y)], fill="#ffdb67")
    image.save(poster)
    manifest = {"source": source.name, "source_sha256": fingerprint, "settings": settings,
                "playback_copy": destination.name, "bytes": destination.stat().st_size,
                "sha256": hashlib.sha256(destination.read_bytes()).hexdigest(), "source_preserved": True}
    cache.write_text(json.dumps(manifest, indent=2), encoding="utf8")
    return destination, poster
