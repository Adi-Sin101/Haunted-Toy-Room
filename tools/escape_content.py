"""Keep the academic content aligned with the eight-stage story implementation.

The original report text (showcase_content.py) describes earlier versions of the mission. revise()
replaces every story-specific paragraph, table and figure with the current story:
night outside -> 257 puzzle -> toy chest -> Buzz's bedroom -> wardrobe rescue -> laser escape ->
morning -> free exploration.
"""

STAGES = [
    ["0 Night outside", "Penny explores the garden and walks in; the main door locks behind her", "Crane shot, player control, porch support heights, hinge easing, fog"],
    ["1 257 puzzle", "Train 2, clock 5, blocks 7; the keypad opens the Toy Room", "Proximity input, digit geometry, cylinder-cap UVs, door yaw"],
    ["2 Toy chest", "Red wind-up button; Woody, Jessie and Bullseye climb out alive", "Hinged lid, eased transitions with a hop arc, gestures"],
    ["3 Buzz's room", "The toys follow Penny downstairs; she opens the door on the left", "Doorway-node navigation, stair support, hinge rotation"],
    ["4 Cupboard rescue", "Jessie rides Bullseye, he jumps, she opens the wardrobe; Buzz flies out", "Re-parenting, body-joint jump arc, reach pose, flight"],
    ["5 Locked main door", "Penny tries the door; Buzz aims and the player fires L", "Nearest-hit laser, rigid-body debris, collision removal"],
    ["6 Morning escape", "Everyone leaves; night turns into morning; the camera pulls back", "Clock-driven sky, sun, fog density and a cinematic camera"],
    ["7 Free exploration", "The rescued toys stay outside; every control remains available", "Independent ownership, flight, mounting in daylight"],
]

OWNERSHIP = (
    "Two layers run the game. The player controls one character (Penny or any freed toy); the story layer moves the rest. Freed toys "
    "accompany Penny in fixed formation slots, so taking one over never moves another; between floors they route through thirteen "
    "doorway nodes using Floyd-Warshall first steps. Followers do not block each other, but walls, doors, furniture and blocks still "
    "stop everyone. Selecting a mounted rider or horse detaches the pair. 0 releases control, N selects full manual mode, Enter "
    "interacts, L fires Buzz's laser once he is in position, Y skips the opening shot and Shift+N replays the story."
)

PUZZLE_OBJECTS = (
    "The wooden train uses box chassis and cab pieces, a horizontal cylindrical boiler, chimney and cylinder wheels. "
    "Two separate cars and a raised seven-segment 2 identify its clue. Seven coloured cubes form the block clue's 7. "
    "The hallway clock is wall-sized: a 0.8-unit circular cylinder case hung at eye height and a thin cylinder face. The provided "
    "BMP uses planar cap UVs; a negative v repeat corrects printed orientation after rotation. A small brass plate below the "
    "face carries a raised 5. The combination keypad has a solid wood housing, steel plate, ten raised buttons, box-strip digit "
    "glyphs and a cylinder confirmation button. Its editable three-digit display is also exposed in the HUD."
)
RESCUE_OBJECTS = (
    "The toy chest is hollow (floor board and four solid walls) with iron brackets and bands, brass rivets, a keyhole plate and a "
    "front panel carrying a procedural painted texture: worn red planks, stars, clouds and a rocket. Its lid is a hinge joint holding a "
    "deep frame box and a cylinder dome whose lower half lies inside the frame, so the lid stays curved when open. The red wind-up "
    "button pulses and a red story light follows it. Pressing it swings the lid to -105 degrees; Woody, Jessie and Bullseye climb to "
    "the rim and hop out with the eased transition (a 0.4 sin(pi t) arc), then cheer. Buzz's bedroom under the Toy Room is ordinary: "
    "star wallpaper, a patchwork star quilt, a bookcase with a globe, rocket and teddy bear, a nightstand lamp (the room's light), a "
    "star rug and a football. The wardrobe has a hollow carcass, an arched crown with finials and two hinged doors with arched panels, "
    "cut-out gold stars and knobs 2.65 units up, too high for Penny; a green glow leaks from the gap while Buzz is inside."
)
ESCAPE_OBJECTS = (
    "Bullseye's jump lifts his body joint, and so the saddle and a mounted Jessie, by 0.75 sin(pi t) over 0.9 s while his root stays "
    "on the floor; Jessie's reach pose raises her right arm. Buzz then flies out on four waypoints. At the entrance the padlock rides "
    "on the door hinge; the laser hides the door and releases six wood boards that the fixed-step solver drops onto the porch deck."
)
GUARDS = (
    "Every stage advances only on its real condition. Penny must actually walk through the open front door before it locks. The "
    "keypad requires all three inspected clues and 257; a wrong code shows Incorrect Code and clears only the digits. The chest opens "
    "when Enter is pressed within 3.2 units of it. Buzz's door opens with Enter beside it on the ground floor. The wardrobe opens only "
    "when a mounted Bullseye is within 1.4 units of the spot beside it and his jump lift exceeds 0.45: Penny's Enter there asks Jessie "
    "and Bullseye for help, or the player can mount (R), ride and jump (L) themselves. The main door must be tried (Enter, or standing "
    "at it for one second). It breaks only after Buzz's nearest laser hit has been the real door leaf for more than 0.65 seconds. "
    "Morning starts when Penny steps outside; free exploration follows once the sixteen-second sunrise has finished and the camera "
    "has pulled back over the house."
)

INTERACTIONS = [
    ["Main door (prologue)", "Open 80 degrees; 2.2 x 3.6 doorway", "Penny inside the corridor: door closes and locks"],
    ["Chest button / bedroom door", "Chest (0.5,0,0.9); corridor wall x=10.5", "Enter within 3.2 / 2.3 units on the right floor"],
    ["Wardrobe knobs", "2.65 above the floor", "Mounted Bullseye within 1.4 units, jump lift > 0.45"],
    ["Main door (escape)", "Leaf at z=9.2", "Door tried; nearest laser hit is the leaf for > 0.65 s"],
    ["Morning", "23:36 to 07:00 in 16 s", "Penny outside; fog = 0.011 x night factor"],
]

NEW_ABSTRACT = (
    "Haunted Toy Room: The Midnight Mission is an interactive three-dimensional graphics project told as an eight-stage story. "
    "At night Penny, a white cat with ginger patches, explores the garden of an abandoned house and walks in; the main door locks "
    "behind her. Upstairs she reads three clues and enters 257 to open the Toy Room, where a glowing red button on an old painted toy "
    "chest brings Woody, Jessie and Bullseye to life. Together they go down to an ordinary bedroom: the wardrobe knob is too high, so "
    "Jessie rides Bullseye, he jumps, she opens it and Buzz flies out. Buzz's laser breaks the locked main door, everyone escapes, the "
    "night turns into morning and the world stays open for free exploration. The player controls any one character; the story layer "
    "drives the others."
)


def revise(blocks, proposal, textures):
    proposal[0][0] = "A haunted house story at night that ends in the morning"
    proposal[0][1] = "Eight stages: garden, 257 puzzle, toy chest, Buzz's room, wardrobe rescue, laser escape, morning, free exploration"
    proposal[0][2] = "StoryDirector / StoryProps / PennyArrival / Environment"
    proposal[7][1] = "27 mapped surfaces, own BMP loader, analytic GPU tracing with BVH, shadows and reflection"
    textures["toy-story-clock"] = ("Hallway clock face", "Provided BMP, custom row/RGB loader and planar cylinder-cap UVs; shared array layer for both renderers.")
    textures["chest-paint"] = ("Toy chest front", "Worn red planks, chipped paint, folded five-point stars, circle clouds, ellipse rocket.")
    textures["star-wallpaper"] = ("Bedroom walls and rug", "Blue stripes; two folded five-point stars per tile.")
    textures["star-quilt"] = ("Patchwork quilt", "4 x 4 patches: navy with a gold star or blue/white plaid; dark seams.")
    textures["star-decal"] = ("Wardrobe stars", "Gold star; alpha = 0 outside it (cut-out).")
    textures["football"] = ("Football", "Black within 17 degrees of the 12 icosahedron vertices.")
    starts = {
        "Haunted Toy Room: The Midnight Mission is an interactive": NEW_ABSTRACT,
        "The recorded concept specifies": "The recorded concept specifies a child's toy room that comes alive at night, primitive-built controllable characters, Jessie riding Bullseye through a parent-child hierarchy, and a morning return. The final implementation realises these features inside a connected two-storey house with a garden, a second furnished bedroom and an eight-stage story with a morning ending. Table 1 maps the proposed functionality to the corresponding final modules.",
        "PennyArrival follows": "The story opens with PennyArrival: a seven-second crane shot from high above the street to a chase position behind Penny, who sits on the pavement looking at the house. The player then controls her in the garden. The physics bounds include the lawn, path, pavement and street; the front walls, fence, gate posts, column bases, railings, shrubs and tree trunks are solid. The porch deck and its three steps have their own support heights. The front door stands open; once Penny is inside the corridor it eases shut and a padlock appears. The stair flight has a 4.5-unit rise over an 8-unit run.",
        "The front door, double toy-room door and stair door use hinge roots": "The front door, double toy-room door, stair door, Buzz's bedroom door, the chest lid and both wardrobe doors use hinge roots: one rotation turns the leaf and every attached part together. Each angle eases exponentially toward its story target. The exterior is drawn outdoors and again after the main door breaks. The ground floor is a union of boxes: corridor, stair landing, Buzz's bedroom and its doorway, so the solid divider and the door leaf decide where a character can pass.",
        "The mission's transition guards": None,
        "Cinematic visibility removes the exterior": "Cinematic visibility removes the exterior during indoor play and restores it when the entrance breaks. The area light follows the camera: the upstairs hall light, the corridor lamp, the nightstand lamp in Buzz's room or the porch lantern. Surface patterns replace book-spine, fence and star geometry where a colour/coverage map is sufficient. Fixed physics work prevents stalls from scheduling unbounded catch-up work. Deterministic media capture uses a fixed simulation step and raw RGB frame recording; the media generator validates frame count before encoding. Benchmark timings are execution observations, not a guarantee for every environment.",
        "The final project is compiled in both Release and Debug": "The final project is compiled in both Release and Debug configurations. The automated checks cover analytic intersections, normal perpendicularity, transform equivalence, shear-safe bounds, contact stability, laser impulse/occlusion, camera sliding, hallway access, the connected stairs, the porch and the Buzz bedroom doorway. A fixed-step rehearsal plays the whole story with real movement, mounting, jumps, interactions and contacts; it logs the real door hit and reaches free exploration only with all five characters outside, in all four raster shading modes, the ray tracer and the Debug build. Live-control runs confirm that driving one toy leaves the others unchanged. Captures exercise all three light types, term isolation, colour-texture toggling, analytic tracing, geometry debug views and object close-ups. The two-minute video is decoded after encoding to check media integrity.",
    }
    substitutions = {
        "Penny walks from the garden into a two-storey house and upstairs. At midnight Woody discovers a stranded car; Jessie rides Bullseye, Buzz clears a doorway obstacle with his laser, and the group escorts the car home. Morning lighting restores the toys to their resting poses.": "",
        "\njumpPosition = mix(start,bed,q²(3-2q)) + (0,sin(pi q),0)": "\nstairSupport = -4.5 + 0.25 clamp(floor(18(z+7)/8)+1,1,18)",
        "\narrivalHour = 20.5 + 2.5 smoothstep(travelledDistance/routeLength)": "\ncamera = mix(street, behindPenny, smoothstep(t/6.5)); porch support = -4.05",
        "all 22 ray-traced maps": "all 27 ray-traced maps",
        "The bed also supplies the final arrival target for Penny; the jump and sleeping pose use her original rig.": "Penny's sitting, walking and sleeping poses all belong to her own rig.",
        "Trees and garden elements give depth to the arrival view": "Trees and garden elements give depth to the opening shot and the morning view",
        "a larger doorway crate is a mission obstacle": "a larger crate stands against the left wall as a collision demonstration",
        "The crate is the story obstacle;": "The crate is a movable demonstration prop;",
        "During activation in the mission, headlights switch on and the car follows its route through the hallway into its home position.": "The car remains an independently driveable prop with moving headlights and distance-driven wheels.",
    }
    captions = {
        "car": "The independently driveable RC car, with four wheels, body panels and headlight lenses.",
        "penny": "Penny's white coat, ginger patches, rounded head and ears, standing in the upstairs hallway.",
        "story-end": "Free exploration in the morning: Penny and all four rescued toys outside the house.",
        "live-control": "Live takeover of Woody while the other toys keep accompanying Penny.",
        "house": "The full house exterior at night as the story opens: porch, garage, fence, trees, lawn, pavement and street.",
        "blocks": "The six-block tower near the front wall. Boxes participate in gravity, separation and laser impulses.",
    }
    result = []
    for block in blocks:
        if block[0] == "heading" and block[1].startswith("CHAPTER IV"):
            result.extend([
                ("heading", "3.12 Story objects: puzzle, toy chest, Buzz's room and wardrobe", 2),
                ("paragraph", PUZZLE_OBJECTS),
                ("figure", "puzzle-objects", "Train, printed circular clock, seven coloured blocks and raised combination keypad."),
                ("paragraph", RESCUE_OBJECTS),
                ("figure", "story-objects", "The closed toy chest, its opening lid, the toys alive, Buzz's bedroom, the closed wardrobe and Bullseye's jump."),
                ("paragraph", ESCAPE_OBJECTS),
                ("figure", "escape-objects", "Buzz flying out of the wardrobe, the locked main door, the nearest-hit laser impact and the board debris on the porch."),
                ("equation", "Fog: T = exp(-density*d); Cfogged = T Crendered + (1-T)(0.055,0.065,0.095); density = 0.011 (1 - daylight) outdoors"),
            ])
        if block[0] == "table" and block[1] == "Story states and visible graphics operations":
            block = (*block[:3], STAGES)
        elif block[0] == "paragraph":
            key = next((k for k in starts if block[1].startswith(k)), None)
            if key == "The mission's transition guards":
                result.extend([("paragraph", GUARDS), ("paragraph", OWNERSHIP)])
                continue
            if key:
                block = ("paragraph", starts[key])
        elif block[0] == "equation" and block[1].startswith("Owned route progress"):
            block = ("equation", "slot i target = Penny + back (1.7 + 1.3 floor(i/2)) ± 0.9 side\nnext node = firstStep[current][goal] (Floyd-Warshall)\njump lift = 0.75 sin(pi t/0.9); hour = h0 + (31 - h0) smoothstep(t/16) mod 24")
        elif block[0] == "figure" and block[1] in captions:
            block = ("figure", block[1], captions[block[1]])
        if block[0] in ("paragraph", "equation"):
            text = block[1]
            for old, new in substitutions.items():
                text = text.replace(old, new)
            block = (block[0], text.replace("  ", " "))
        result.append(block)
    blocks[:] = result
