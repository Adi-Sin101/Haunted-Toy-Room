# Haunted Toy Room: the story

The game has two layers.

1. **Player layer.** The player controls one character at a time: Penny, Woody, Jessie, Bullseye,
   Buzz or the RC car (click it, or keys 1-5; 9 or the PENNY button takes Penny over or hands her back
   at any time; 0 releases).
2. **Story layer.** `StoryDirector` moves every character the player is not controlling. Freed toys
   accompany Penny; Jessie and Bullseye perform the wardrobe rescue; Buzz flies into position at the
   locked door.

`StoryDirector` owns one eight-state machine. Each state accepts only its own transition, and every
transition needs its real condition (no timeouts stand in for player actions).

| State | What happens | Advances when |
| --- | --- | --- |
| PROLOGUE | Night outside. Penny explores the garden; the front door stands open | Penny is inside the corridor: the door closes and locks |
| PUZZLE | Upstairs hallway: train 2, clock 5, blocks 7 | All three clues inspected and 257 submitted |
| TOY_CHEST | Penny presses the red wind-up button; Woody, Jessie and Bullseye climb out | All three have landed ("The toys are alive!") |
| BUZZ_ROOM | Everyone goes downstairs together | Enter at the ordinary door on the corridor's left |
| WARDROBE | Enter right in front of the doors: the knob is too high, so Jessie rides Bullseye, he jumps, she opens it | Buzz has flown out and landed |
| FINAL_ESCAPE | Penny tries the main door; Buzz aims; the player presses L | Real laser hits break the door and Penny steps outside |
| MORNING | Everyone leaves; night turns into morning; the camera pulls back | Sunrise (16 s) finished and the pull-back shown |
| FREE_EXPLORE | The rescued toys stay outside; every control remains | (open ended; Shift+N replays) |

The toys cannot be selected while they are still inside the chest or the wardrobe. Manual mode
(`--manual`, or N) is a sandbox: every toy stands in the Toy Room, every door is open and the story
layer is paused.

See [the illustrated object, equation and implementation explanation](20-escape-gameplay.md) for the
chest, the bedroom and wardrobe, the follower graph, the jump, the laser, the morning and the tests.
