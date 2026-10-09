"""Report content for the outdoor sky, the smoothed follow camera, solid characters and Penny control.

update() runs after escape_content.revise(): it replaces the paragraphs these features made obsolete
and inserts two sections into Chapter IV (renumbering the sections after them). Equation blocks carry
their own LaTeX as a third element, so the numbered list in latex_equations.py stays untouched.
"""

SKY_WINDOW = (
    "Environment::UpdateSky maps the 24-hour clock angle a = (h-6)pi/12 onto two skies. Indoors, small "
    "emissive sun and moon spheres orbit behind the bedroom window in front of an unlit starry backdrop, "
    "and the directional light points from them into the room. Outdoors the backdrop, those spheres and "
    "the flat roof silhouettes are hidden and a procedural sky dome takes over (Section 4.5). A smooth "
    "daylight factor blends ambient light, sky colours and directional-light colour for both."
)

CAMERA = (
    "For picking, a screen point is converted to normalised device coordinates and a camera ray. Each "
    "candidate's inverse model matrix maps that ray to object space, and the nearest positive primitive "
    "intersection chooses the object's owner. Camera movement is collision-aware: a sightline test pulls "
    "the orbit and follow camera in front of walls at once and releases it gradually. The follow camera "
    "eases its focus with frame-rate independent exponential smoothing, slowest in height, so the 0.25 "
    "stair treads no longer jolt the view (Section 4.8). GLFW supplies the event callbacks and input states [2]."
)

OWNERSHIP = (
    "Two layers run the game. The player controls one character (Penny or any freed toy); the story layer "
    "moves the rest. The PENNY row of the corner panel (or key 9) takes Penny over, with the follow camera "
    "behind her wherever she is, or hands her back to the simulation. Freed toys accompany Penny in fixed "
    "formation slots and route between floors through doorway nodes using Floyd-Warshall first steps. "
    "Characters are solid to each other: overlaps are resolved by priority and story walkers steer around "
    "bodies ahead (Section 4.8). Selecting a mounted rider or horse detaches the pair. 0 releases control, "
    "N selects full manual mode, Enter interacts, L fires Buzz's laser once he is in position, Y skips the "
    "opening shot and Shift+N replays the story."
)

GUARDS = (
    "Every stage advances only on its real condition. Penny must actually walk through the open front door "
    "before it locks. The keypad requires all three inspected clues and 257; a wrong code clears only the "
    "digits. The chest opens when Enter is pressed within 3.2 units of it, Buzz's door with Enter beside it. "
    "The wardrobe rescue starts only with Enter within 1.6 units of the point in front of its doors; "
    "elsewhere in the bedroom nothing happens. The doors open when a mounted Bullseye is within 1.4 units "
    "of that point and his jump lift exceeds 0.45, whether the story or the player (R, ride, L) performs it. "
    "The main door must be tried; it breaks only after Buzz's nearest laser hit has been the real door leaf "
    "for more than 0.65 seconds. Morning starts when Penny steps outside; free exploration follows the "
    "sixteen-second sunrise and the camera's pull-back over the house."
)

SKY_SECTION = [
    ("heading", "4.5 Outdoor sky dome, sun and moon", 2),
    ("paragraph",
     "The sky is a function of direction only, skyRadiance(d) in sky.glsl, shared by both renderers. A "
     "horizon-to-zenith gradient (exponent 0.45 on elevation) carries an orange sunset band toward the sun's "
     "azimuth. Stars come from a 3D grid of cells over the unit sphere: a cell whose hash exceeds 0.9965 "
     "holds one twinkling star that fades near the horizon and at dawn. The moon is a 1.7 degree disc with "
     "fractal-noise maria, limb darkening and a halo; clouds are four octaves of value noise projected on a "
     "plane above the camera and lit by the sun or, faintly, the moon; the sun is a sharp disc with two "
     "power-law glows. In raster mode a full-screen triangle on the far plane (z = w) is drawn after the "
     "opaque pass with a less-or-equal depth test, so only uncovered pixels run the sky shader. In the ray "
     "tracer every missed primary, reflected or transmitted ray returns the same function, so mirrors and "
     "glass show the real sky. The fog colour equals the horizon colour, so distant ground fades into the sky."),
    ("equation",
     "sunDir = normalize(-60 cos a, 55 sin a, 30); moonDir = normalize(60 cos a, -55 sin a, 30)\n"
     "L0 = normalize(mix(windowDir, -skyDir, w)); w += (outside - w)(1 - exp(-8 dt))\n"
     "P_sun = orthographic(30, 30, 1, 150) lookAt(c - 90 L, c); v = PCF3x3(depth test) faded by w",
     r"""\mathbf s=\operatorname{normalize}(-60\cos a,55\sin a,30),\qquad\mathbf m=\operatorname{normalize}(60\cos a,-55\sin a,30)\\
\mathbf L_0=\operatorname{normalize}\big(\operatorname{mix}(\mathbf d_{\rm window},-\mathbf d_{\rm sky},w)\big),\qquad w\leftarrow w+(o-w)(1-e^{-8\Delta t})\\
P_{\rm sun}=\operatorname{ortho}(30,30,1,150)\,\operatorname{lookAt}(\mathbf c-90\mathbf L,\mathbf c),\qquad v=1-w\Big(1-\tfrac19\sum_j\mathbf 1\{q_z-\beta\le D_j\}\Big)"""),
    ("paragraph",
     "The sun crosses the southern sky in front of the house and the moon the opposite half of the arc, so "
     "the facade faces the sun by day and the moon at night. Outdoors the directional light comes from that "
     "visible body; indoors it keeps the window direction, so the rooms look as before. The blend w eases "
     "over 0.3 s through the front door and snaps on a camera jump. Ray-traced sun and moon shadows use the "
     "existing shadow rays; the raster path adds a 2048-squared orthographic depth map over 60 x 60 units "
     "ahead of the camera, snapped to whole texels so edges do not crawl, with slope-scaled bias and 3 x 3 PCF."),
    ("figure", "sky-before", "Before (top left): a flat backdrop plane north of the house. After: the sky dome by day (raster, ray traced), at night (raster, ray traced) and the moon with its maria."),
    ("figure", "sky-day-raster", ""), ("figure", "sky-day-rt", ""), ("figure", "sky-night-raster", ""),
    ("figure", "sky-night-rt", ""), ("figure", "sky-moon", ""),
]

SOLID_SECTION = [
    ("heading", "4.8 Solid characters and the follow camera", 2),
    ("paragraph",
     "Penny reaches from her tail at -0.8 to her nose at +1.0 and Bullseye from -0.9 to +1.7, but their "
     "boxes covered only about half of that, so heads passed through walls. One long box would swell by up "
     "to 41 percent at 45 degrees and could not turn in the 3-unit corridor. Both now use a spine of square "
     "boxes along the body; every segment is swept with the same rotation and the most restrictive result "
     "is kept, so the body stops with its nose at a wall and turning toward a wall pushes it back. A thin "
     "solid divider turns the zero-thickness wall between corridor and stair flight into a real wall."),
    ("equation",
     "focus.xz += (t.xz - focus.xz)(1 - exp(-14 dt)); focus.y += (t.y - focus.y)(1 - exp(-5 dt))\n"
     "reach = allowed if allowed < reach else reach + (allowed - reach)(1 - exp(-2.5 dt))\n"
     "overlap (ox, oz): lower body moves by the shorter push, the other axis if pinned; higher body takes the rest",
     r"""\mathbf f_{xz}\leftarrow\mathbf f_{xz}+(\mathbf t_{xz}-\mathbf f_{xz})(1-e^{-14\Delta t}),\qquad f_y\leftarrow f_y+(t_y-f_y)(1-e^{-5\Delta t})\\
r\leftarrow\begin{cases}r_{\rm allowed},&r_{\rm allowed}<r\\ r+(r_{\rm allowed}-r)(1-e^{-2.5\Delta t}),&\text{otherwise}\end{cases}\\
\boldsymbol\delta_{\rm low}=\operatorname{slide}\big(\min(o_x,o_z)\,\mathbf e\big),\qquad\boldsymbol\delta_{\rm high}=-\big(\|\boldsymbol\delta\|-\boldsymbol\delta_{\rm low}\cdot\mathbf e\big)\mathbf e"""),
    ("paragraph",
     "A separation pass resolves every overlap between characters by strict priority: the driven character, "
     "characters on a story task, Penny, then the followers. The lower body yields the whole push, sliding "
     "against the house (stepping aside along the other axis when pinned); the higher one takes the rest. "
     "Equal shares cancel in a doorway and jam; strict priorities always have a winner. Story walkers also "
     "steer around bodies ahead. The toys now wait behind Buzz's firing position, the porch is left only by "
     "its steps and in the morning everyone walks to their own place on the lawn; the scripted story reaches "
     "free exploration at 70 s (74.6 s before)."),
    ("figure", "stair-corner-before", "Before: Penny's front half inside the corridor wall and Buzz flying through Bullseye and Jessie. After: the same moments with spine collision, the solid divider and priority contact."),
    ("figure", "stair-corner-after", ""), ("figure", "crowd-before", ""), ("figure", "crowd-after", ""),
    ("paragraph",
     "The follow camera used to aim at the raw character position, so each 0.25 stair tread became a jolt, "
     "and the stairwell sightline moved it in and out on alternate frames. It now eases focus and position "
     "with frame-rate independent rates (slowest in height) and releases a wall pull gradually."),
    ("table", "Follow camera on stairs and porch steps (same scripted run; per-frame second differences)",
     ["Measure", "Before", "After"],
     [["Pitch acceleration, RMS", "2.42 deg/frame2", "0.14 deg/frame2"],
      ["Pitch jolts above 0.5 deg/frame2", "82", "1"],
      ["Camera height acceleration, RMS", "0.047 units/frame2", "0.022 units/frame2"]]),
    ("paragraph",
     "The PENNY row of the corner panel shows who drives her (YOU CONTROL HER or SIMULATION) and toggles on a "
     "click or key 9, so a released Penny can be taken back from anywhere; ray-tracing bounces moved to "
     "Ctrl+9. The wardrobe no longer starts the rescue 2.5 s after Penny enters the bedroom: only Enter right "
     "in front of its doors does."),
]

RENUMBER = {
    "4.5 Coordinated animation and interaction": "4.6 Coordinated animation and interaction",
    "4.6 Collision and frame-time control": "4.7 Collision and frame-time control",
    "4.7 Optimisation decisions": "4.9 Optimisation decisions",
    "4.8 Verification and achieved objectives": "4.10 Verification and achieved objectives",
}

ABSTRACT_ADDITION = (
    " Outdoors a procedural sky dome, shared by the rasteriser and the ray tracer, supplies the sun, moon, "
    "stars and clouds that light and shadow the garden, and every character is a solid body."
)


def update(blocks):
    replace = {
        "Environment::UpdateSky maps the 24-hour clock": SKY_WINDOW,
        "For picking, a screen point is converted": CAMERA,
        "Two layers run the game.": OWNERSHIP,
        "Every stage advances only on its real condition.": GUARDS,
    }
    result = []
    for block in blocks:
        if block[0] == "heading" and block[1] in RENUMBER:
            title = RENUMBER[block[1]]
            if title.startswith("4.6 "):
                result.extend(SKY_SECTION)
            if title.startswith("4.9 "):
                result.extend(SOLID_SECTION)
            block = ("heading", title, block[2])
        elif block[0] == "paragraph":
            key = next((k for k in replace if block[1].startswith(k)), None)
            if key:
                block = ("paragraph", replace[key])
            elif block[1].startswith("Haunted Toy Room: The Midnight Mission is an interactive"):
                block = ("paragraph", block[1] + ABSTRACT_ADDITION)
        result.append(block)
    # Figures with an empty caption only feed a montage (compact_report groups them under the first one).
    blocks[:] = [b if not (b[0] == "figure" and b[2] == "") else (b[0], b[1], b[1].replace("-", " ")) for b in result]
