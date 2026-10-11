"""Report content for distance-based companion following and the manual day / night override.

update() runs after outdoor_physics_content.update(): it inserts two sections into Chapter IV
(renumbering the two after them), corrects the paragraph and the follower formula these features made
obsolete and extends the abstract. Equation blocks for the new sections carry
their own LaTeX as a third element, so the numbered list in latex_equations.py stays untouched.
"""

OWNERSHIP = (
    "Two layers run the game. The player controls one character (Penny or any freed toy); the story layer "
    "moves the rest. The PENNY row of the corner panel (or key 9) takes Penny over, with the follow camera "
    "behind her wherever she is, or hands her back to the simulation. Freed toys follow Penny by distance "
    "along her path (Section 4.9) and route between floors through doorway nodes using Floyd-Warshall first "
    "steps. Characters are solid to each other: overlaps are resolved by priority and story walkers steer "
    "around bodies ahead (Section 4.8). Selecting a mounted rider or horse detaches the pair. 0 releases "
    "control, N selects full manual mode, Enter interacts, L fires Buzz's laser once he is in position, / "
    "switches day and night (Section 4.10), Y skips the opening shot and Shift+N replays the story."
)

FOLLOW_FORMULA = (
    "slot i = trail point b_i back along Penny's path + s_i right of it (Section 4.9)\n"
    "next node = firstStep[current][goal] (Floyd-Warshall)\n"
    "jump lift = 0.75 sin(pi t/0.9); hour = h0 + (31 - h0) smoothstep(t/16) mod 24"
)

FOLLOW_SECTION = [
    ("heading", "4.9 Distance-based companion following", 2),
    ("paragraph",
     "CompanionFollow replaces per-frame slots from Penny's facing with one group anchor and a small state "
     "per toy. The anchor stays frozen while Penny stands, turns or moves "
     "less than 2.0; beyond that it tracks her, dropping a breadcrumb every 0.3, until she has stood still for "
     "0.35 s. Each slot lies a fixed distance back along that trail and to the right of it: Woody and Jessie "
     "0.95 either side 1.7 behind, Bullseye 4.2 behind, Buzz 1.5 aside and 3.3 back, clear for flight. The "
     "trail is her own route, so slots on stairs, in the corridor and in doorways are walkable."),
    ("equation",
     "slot q_i = T(b_i) + s_i r(b_i); the anchor follows once |Penny - anchor| > 2.0 and freezes after 0.35 s standing\n"
     "v_cap = 2.1 + 1.9 min(1, d/4.5); v* = clamp(sqrt(2*3*max(0, d - 0.3)), 0.35, v_cap); v += clamp(v* - v, -4 dt, 4 dt)\n"
     "Settled -> Approaching: following and d > 1.2; Approaching -> Settled: standing and (d <= 0.3 or |Penny - c| < 1.0 or 1 s without progress)",
     r"""\mathbf q_i=\mathbf T(b_i)+s_i\,\hat{\mathbf r}(b_i),\qquad\text{follow}\iff\|\mathbf p_P-\mathbf a\|>2.0\\
v_{\rm cap}=2.1+1.9\min(1,d/4.5),\qquad v^\ast=\operatorname{clamp}\big(\sqrt{6\max(0,d-0.3)},\,0.35,\,v_{\rm cap}\big)\\
v\leftarrow v+\operatorname{clamp}(v^\ast-v,\pm4\Delta t),\quad\text{start: }d>1.2,\quad\text{settle: }d\le0.3\lor\|\mathbf p_P-\mathbf c\|<1\lor\text{stall}"""),
    ("paragraph",
     "A settled toy issues no movement; separate start and stop distances stop it twitching. It also stops "
     "within 1.0 of a standing Penny, or after a second blocked near its slot. Values live in "
     "StoryDirector::followSettings and movement uses the existing autopilot and collision pass. The driven "
     "toy and a mounted Jessie are left out, so input and following never mix; a released toy rejoins only "
     "if more than 2.0 from its slot. Each toy changes mode at most once per stage; free exploration is "
     "reached at 73.4 s (70.2 s before)."),
]

DAYNIGHT_SECTION = [
    ("heading", "4.10 Manual day/night override", 2),
    ("paragraph",
     "The story owns the clock, so a player switch must not change it. Environment keeps two hours: hour, "
     "the clock the story sets and the clock hands show, and skyHour, read by every sky and lighting "
     "function (sun, moon, daylight, ambient, sky and fog colours, stars, sky light, neighbouring houses "
     "and the lamp automation). Without an override skyHour equals hour, so sunset and sunrise are "
     "unchanged; an override eases it to 12:00 or 00:00 over 1.5 s the shorter way round. Both renderers "
     "read the same functions. The / key or the DAY / NIGHT button beside the settings button switches; the "
     "button shows the look, a MANUAL tag and the action, and fires only on a press and release inside it. "
     "Asking for what the clock shows clears the override. The rehearsal gives identical stages, timings "
     "and positions with and without the switch."),
    ("equation",
     "target = 12 (Day), 0 (Night) or hour (no override); span = ((target - h0 + 36) mod 24) - 12\n"
     "skyHour = (h0 + span smoothstep(min(1, t/1.5))) mod 24; daylight = smoothstep(-0.1, 0.25, sin((skyHour - 6) pi/12))",
     r"""h^\ast=\begin{cases}12,&\text{Day}\\0,&\text{Night}\\h,&\text{no override}\end{cases},\qquad\Delta=\big((h^\ast-h_0+36)\bmod24\big)-12\\
h_{\rm sky}=\big(h_0+\Delta\operatorname{smoothstep}(\min(1,t/1.5))\big)\bmod24,\qquad D=\operatorname{smoothstep}\big(-0.1,0.25,\sin\tfrac{(h_{\rm sky}-6)\pi}{12}\big)"""),
]

RENUMBER = {
    "4.9 Optimisation decisions": "4.11 Optimisation decisions",
    "4.10 Verification and achieved objectives": "4.12 Verification and achieved objectives",
}

ABSTRACT_ADDITION = (
    " The freed toys follow Penny by distance and settle when she stops, and a day/night switch changes "
    "the sky and lighting without touching the story clock."
)

def update(blocks):
    result = []
    for block in blocks:
        if block[0] == "heading" and block[1] in RENUMBER:
            title = RENUMBER[block[1]]
            if title.startswith("4.11 "):
                result.extend(FOLLOW_SECTION)
                result.extend(DAYNIGHT_SECTION)
            block = ("heading", title, block[2])
        elif block[0] == "paragraph":
            text = block[1]
            if text.startswith("Two layers run the game."):
                block = ("paragraph", OWNERSHIP)
            elif text.startswith("Haunted Toy Room: The Midnight Mission is an interactive"):
                block = ("paragraph", text + ABSTRACT_ADDITION)
        elif block[0] == "equation" and block[1].startswith("slot i target = Penny"):
            # Text form only: its numbered LaTeX (the route step) is still correct.
            block = ("equation", FOLLOW_FORMULA)
        result.append(block)
    blocks[:] = result
