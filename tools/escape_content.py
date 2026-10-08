"""Keep the academic content aligned with the connected escape implementation."""

STAGES = [
    ["Prologue", "Penny enters the house at night", "Waypoints, hinges, stair slope and camera"],
    ["Puzzle", "Inspect train 2, clock 5, blocks 7; enter 257", "Proximity input, digit geometry, cap UVs, door yaw"],
    ["Toy rescue", "Two low switches; mounted Jessie reaches the high platform", "Hierarchy, pose transitions, lever rotation, raised gate"],
    ["Buzz rescue", "Activate the rear holding-area release", "Translucent barrier and collision removal"],
    ["Escape", "Descend; ghost follows; Buzz breaks the entrance", "Ground support, flight, nearest-hit laser, debris"],
    ["Win", "All five are outside and idle in a night view", "Actual-position guard; rest pose preserves placement"],
]

OWNERSHIP = (
    "Selection transfers input ownership to one character while other actors continue their routes and actions. "
    "The director skips motion and special-state writes for the owned actor; a separate virtual cursor keeps route progress. "
    "The final escape guard still requires every character's actual position outside. Releasing ownership chooses a waypoint "
    "on the actor's current floor and steers there without teleporting. A manually aimed Buzz laser follows the same real-hit "
    "rule as his scripted flight. Scripted actors ignore the owned actor as a contact obstacle, while its furniture and wall "
    "contacts remain active. Selecting a mounted rider or horse detaches the pair; remounting deliberately controls them together. "
    "Press 0 to release, N for full manual mode, Ctrl+0 for Penny, Enter to interact, Y to skip arrival, and Shift+N to restart."
)

PUZZLE_OBJECTS = (
    "The wooden train uses box chassis and cab pieces, a horizontal cylindrical boiler, chimney and cylinder wheels. "
    "Two separate cars and a raised seven-segment 2 identify its clue. Seven coloured cubes form the block clue's 7. "
    "The hallway clock has a circular cylinder case and a thin cylinder face. The provided BMP uses planar cap UVs; "
    "a negative v repeat corrects printed orientation after rotation. A raised 5 below the face keeps the clue readable. "
    "The combination keypad has a solid wood housing, steel plate, ten raised buttons, box-strip digit glyphs and a "
    "cylinder confirmation button. Its editable three-digit display is also exposed in the HUD."
)
RESCUE_OBJECTS = (
    "The rescue gate combines nine thin cylinders and a box rail. Its parent joint raises after two low switches and "
    "becomes hidden after the high switch. Each switch has a box backplate and a cylinder lever rotating from 20 to -45 degrees. "
    "Jessie's support platform is a solid 2.0 by 1.3 by 1.6 box; dismounting uses the existing eased rig transition. "
    "Buzz's holding area reuses the rear room floor, two box partitions and a cyan translucent barrier. Its red box release "
    "switch removes the barrier from drawing and contact tests. The entrance note is a thin paper box, accompanied by "
    "the arrival objective explaining that the toys need help."
)
ESCAPE_OBJECTS = (
    "The front-door padlock belongs to the existing hinge: one metal body box, two upright cylinders and a rotated cylinder "
    "crown move with the door. Laser impact hides this assembly and activates six wood-textured box fragments. Gravity, "
    "contact separation and angular impulses use the existing fixed-step solver. The eighteen rendered stair treads each "
    "rise 0.25 units; the support function matches each tread rather than letting a character sink through a ramp. Grounded "
    "actors remain on the correct floor, while Buzz retains vertical flight. Final rest-pose restoration preserves each root "
    "transform so the ending cannot move the cast back indoors."
)
GUARDS = (
    "The keypad requires all three inspected clues and the code 257. A wrong code shows Incorrect Code and clears only "
    "the digits. Penny must approach both low switches. Jessie must have ridden Bullseye beneath the high switch, "
    "dismounted onto its platform and completed her transition before Enter activates it. The rear release belongs to Penny. "
    "At the ground-floor entrance, Enter starts Buzz's flight. The door breaks only after his nearest laser hit is the actual "
    "door for more than 0.65 seconds. Six preallocated boards receive impulses; the ghost pursues at 1.6 units per second. "
    "The ending requires the broken entrance and Penny plus all four rescued toys physically outside."
)


def revise(blocks, proposal, textures):
    proposal[0][1] = "Night arrival, clue puzzle, two rescues, pursuit and outdoor escape; daylight remains available for comparison"
    textures["toy-story-clock"] = ("Hallway clock face", "Provided BMP, custom row/RGB loader and planar cylinder-cap UVs; shared array layer for both renderers.")
    substitutions = {
        "follows fifteen waypoints": "follows twelve waypoints",
        "A 0.7-second eased interpolation with a sine lift produces the hop onto the bed. ": "The arrival ends at the upper hallway, before the locked puzzle door. ",
        "Penny; the jump and sleeping pose use her original rig.": "Penny; sitting and walking poses reuse her original rig.",
        "\njumpPosition = mix(start,bed,q?(3-2q)) + (0,sin(pi q),0)": "\nstairSupport = -4.5 + 0.25 clamp(floor(18(z+7)/8)+1,1,18)",
        "At midnight Woody discovers a stranded car; Jessie rides Bullseye, Buzz clears a doorway obstacle with his laser, and the group escorts the car home. Morning lighting restores the toys to their resting poses.":
        "Penny solves a hallway combination puzzle, frees the toys using three switches and releases Buzz from a rear holding area. A ghost pursues Penny downstairs. Buzz's actual laser impact breaks the locked entrance, and all five characters escape into the garden.",
        "and a morning return": "and a quiet ending",
        "During activation in the mission, headlights switch on and the car follows its route through the hallway into its home position.": "The car remains an independently driveable demonstration prop with moving headlights and distance-driven wheels.",
        "Route progress also drives sunset. The house exterior is hidden after the stairs are reached, and the downstairs interior is hidden after the arrival finishes.": "Route progress keeps the arrival between 20:30 and 23:00. The exterior is culled indoors and returns after the entrance breaks; the downstairs remains connected for the escape. Grounded actors use support heights matching all eighteen rendered treads.",
        "a larger doorway crate is a mission obstacle": "a larger crate provides a collision demonstration clear of the hallway route",
        "The crate is the story obstacle;": "The crate is a movable demonstration prop;",
        "Cinematic visibility hides downstairs geometry after arrival while retaining its inventory records.": "The downstairs remains connected; the stair door stays open, the toy-room door opens after the puzzle and the front door remains solid until laser impact.",
        "Cinematic visibility removes the exterior and downstairs geometry when the arrival no longer exposes them.": "Cinematic visibility removes the exterior during indoor play and restores it when the entrance breaks; the connected downstairs remains available.",
        "The full arrival/mission replay reaches The End, logs mounting and dismounting, and restores the driveable toys to their expected home positions.": "The fixed-step escape rehearsal uses normal movement, mounting, interaction and contacts, logs the real door hit and reaches WIN only when all five actors are outside.",
    }
    result = []
    for block in blocks:
        if block[0] == "heading" and block[1].startswith("CHAPTER IV"):
            result.extend([
                ("heading", "3.12 Puzzle, rescue and escape objects", 2),
                ("paragraph", PUZZLE_OBJECTS),
                ("figure", "puzzle-objects", "Train, printed circular clock, seven coloured blocks and raised combination keypad."),
                ("paragraph", RESCUE_OBJECTS), ("paragraph", ESCAPE_OBJECTS),
                ("figure", "escape-objects", "Rescue platform, holding barrier, physical padlock, laser contact, debris and connected stair treads."),
                ("table", "Interaction and contact conditions", ["Mechanism", "Location / value", "Required condition"], [
                    ["Low switches", "(7.1,0.75,2) and (7.1,0.75,5.8)", "Penny within 1.7 horizontal units"],
                    ["High switch", "(2.5,3.25,4.5); platform top y=1.3", "Jessie: mounted approach, dismounted, y>1, within 1.5 units; transition finished"],
                    ["Buzz release", "(2.8,0.8,-3)", "Penny within 1.8 horizontal units"],
                    ["Entrance", "(12,-4.5,9.2)", "Penny nearby and downstairs; Buzz laser's nearest hit is the door for >0.65 s"],
                    ["Stair flight", "18 treads; rise 0.25; width 3", "Support matches geometry; oversized rotated proxy centres safely"],
                    ["Escape completion", "Door broken; z>13 and y<-0.3", "All five actual character positions pass; virtual progress cannot substitute"],
                ]),
                ("paragraph", "Exponential depth fog blends rendered RGB with (0.055,0.065,0.095). Transmission is exp(-density times distance): density is 0.008 during pursuit, 0.010 outdoors at night and zero otherwise. Raster uses eye-to-surface distance; analytic tracing uses primary-hit distance after accumulating local, reflected and transmitted colour. This is a depth cue rather than participating-medium transport."),
                ("equation", "T = exp(-density*d); Cfogged = T Crendered + (1-T) Cfog"),
            ])
        if block[0] == "table" and block[1] == "Story states and visible graphics operations":
            block = (*block[:3], STAGES)
        elif block[0] == "paragraph" and block[1].startswith("The mission's transition guards"):
            result.extend([("paragraph", GUARDS), ("paragraph", OWNERSHIP)])
            continue
        elif block[0] == "equation" and block[1].startswith("Owned route progress"):
            block = ("equation", block[1].split("Scene gate")[0] + "Final escape gate = broken door AND every actual character outside")
        elif block[0] == "figure" and block[1] == "car":
            block = ("figure", "car", "The independently driveable RC car, with four wheels, body panels and headlight lenses.")
        elif block[0] == "figure" and block[1] == "story-end":
            block = ("figure", "story-end", "The completed escape at night, with Penny and all four rescued toys physically outside.")
        elif block[0] == "figure" and block[1] == "live-control":
            block = ("figure", "live-control", "Live character takeover while the remaining actors continue their escape routes.")
        if block[0] in ("paragraph", "equation"):
            text = block[1]
            for old, new in substitutions.items():
                text = text.replace(old, new)
            block = (block[0], text)
        result.append(block)
    blocks[:] = result
