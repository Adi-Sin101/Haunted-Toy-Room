# Haunted Toy Room: escape progression

`StoryDirector` owns one six-state progression machine. The night arrival ends at the upper hallway;
Penny then solves the combination, frees the toys, releases Buzz and leads the cast outside.

| State | Guard |
| --- | --- |
| PROLOGUE | Arrival completes or Y skips |
| PUZZLE | All three clues inspected and 257 submitted |
| TOY_RESCUE | Two Penny switches; Jessie rides, dismounts on the platform and activates the high switch |
| BUZZ_RESCUE | Penny activates the rear release |
| FINAL_ESCAPE | Entrance breaks from actual laser hits and all five actors are outside |
| WIN | Idle cast, wide outdoor night camera and an inspectable scene |

Advance accepts each event only from its corresponding state. The keypad owns its own digit/clue
state; the director owns progression. Door visibility and solid flags follow the same transitions.
The front-door hit timer cannot be substituted by a timeout or a synthetic completion event.

Select a character for live input while the other escape routes continue. Zero releases it; N
switches all routes to manual control. Releasing control rejoins the current floor without
teleporting. A mounted pair can be intentionally controlled together. The final guard always
checks actual positions, including any owned character. Shift+N restores the complete setup.

See [the illustrated object, equation and implementation explanation](20-escape-gameplay.md)
for switch distances, stair support, ray impact, debris, fog, ownership, source functions and tests.
