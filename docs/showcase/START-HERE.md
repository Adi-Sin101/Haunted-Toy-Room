# Haunted Toy Room: The Midnight Mission

Adiba Tahsin | Roll 2107031 | CSE-4102

## Showcase files

| File | Purpose |
| --- | --- |
| [Teacher-Evaluation-QA.md](Teacher-Evaluation-QA.md) | 130 viva questions with answers, equations, current story logic and exact source locations |
| [Teacher-Evaluation-QA.pdf](Teacher-Evaluation-QA.pdf) / [Word](Teacher-Evaluation-QA.docx) | Printable and editable versions of the complete evaluation questionnaire |
| [Project-Report.pdf](Project-Report.pdf) | Finished illustrated academic report: 38-page LaTeX report with one combined cover, a separate complete contents, four chapters starting on new pages, references and object/material summaries |
| [Project-Report.tex](Project-Report.tex) | Editable LaTeX report source |
| [Comprehensive-Implementation-Notes.md](Comprehensive-Implementation-Notes.md) | Complete theory and implementation notes, with every scene node and material |
| [Requirements-Coverage.md](Requirements-Coverage.md) | Teacher requirement mapping to implementations and demonstration evidence |
| [Project-Presentation.pptx](Project-Presentation.pptx) | Editable twelve-slide haunted-house deck with a dedicated texture slide, typeset equations, speaker notes and the recorded walkthrough embedded on the final slide |
| [Project-Presentation.pdf](Project-Presentation.pdf) | Portable slide copy |
| [Presentation-Demo.mp4](Presentation-Demo.mp4) | Silent full-HD playback copy of the supplied recording, embedded on slide 12; the original recording is preserved |
| [Project-Demo.mp4](Project-Demo.mp4) | Two-minute, 24 fps demonstration recorded from the application |
| [Demonstration-Study-Guide.pdf](Demonstration-Study-Guide.pdf) | Printable narration, rehearsal sequence, viva answers and parameter-change practice |
| [Demonstration-Study-Guide.md](Demonstration-Study-Guide.md) | Searchable/editable learning guide |
| [Submission-Package.zip](Submission-Package.zip) | Runnable application, source, documents, media and validation evidence |

The video has explanatory captions; its timed narration is supplied for the presenter to explain live. The final slide embeds Presentation-Demo.mp4, a balanced, silent full-HD copy of the supplied recording. The original two-minute Project-Demo.mp4 and its timed narration guide remain available separately.

Presentation playback uses H.264 at 1920 × 1080, with balanced CRF 21 compression and no audio. The supplied walkthrough runs for 2 minutes 17 seconds. The PowerPoint contains the video internally; the PDF shows its poster frame, with the standalone MP4 available for playback.

## Run and demonstrate

From the project folder:

```powershell
.\Run-Showcase.ps1
.\Run-Showcase.ps1 -Mode Manual -Raster
.\Run-Showcase.ps1 -Mode Mounted -Raster
```

The default launch opens on the street at night; Y skips the crane shot. Walk Penny into the house, inspect the three clues with Enter and submit 257 at the keypad. In the Toy Room press Enter at the chest's red button: Woody, Jessie and Bullseye come alive and follow Penny. Downstairs, Enter opens the door on the left; Enter at the wardrobe asks Jessie and Bullseye for help, and Buzz flies out. At the main door Enter tries it, then L fires Buzz's laser. Walk outside into the morning; free exploration follows. See [the story guide](../20-escape-gameplay.md). Select a freed toy with 1-5 or a click to take live control while the story layer drives the others. W/S moves, A/D turns, Shift runs and Space stops. Press 0 to release; 9 or the PENNY row of the corner panel takes Penny back at any time. Characters are solid to each other. The wardrobe answers only to Enter right in front of its doors. Outdoors, the sky dome supplies the sun, moon, stars and clouds, and the sun or moon lights and shadows the garden in both rendering paths. N switches the scene to full manual mode or resumes the story. Shift+N restarts everything. Selecting a mounted rider or horse detaches the rider for independent control; remounting intentionally controls the connected pair.

F2 selects raster rendering and cycles Flat/Gouraud/Phong/Blinn. F4 switches analytic ray tracing; Ctrl+9 changes its bounce count. F5/F6/F7 toggle illumination terms. F3 toggles textures. F focuses an object; V lists its parts. Tab enables transformations. O enables haunted ambience. H expands the control guide; G hides the interface. Detailed controls are in [the reference](../13-controls.md).

## What is documented

The report contains 35 numbered figures, 12 tables and 31 numbered equation groups in 38 pages, including the outdoor sky dome with sun/moon light and shadows, solid characters (collision spines, priority contact, steering), the smoothed follow camera and the PENNY control. It explains all five primitive families, the scene hierarchy, transformations, camera/projection, directional/point/spot lights, illumination equations, four raster shading models, 27 surface maps plus the white fallback, lamp shadow mapping, BVH/analytic ray tracing, animation, collision, the eight-stage story (garden, 257 puzzle, toy chest, Buzz's bedroom and wardrobe, laser escape, morning) and the player/story layers. The report appendices summarise the object families and representative materials; the comprehensive implementation notes and CSV exports record all 993 scene nodes and every material in detail. Actual exports are in [inventory](inventory/objects.csv); full-resolution illustrations are in `figures/`, `diagrams/` and `inventory/textures/`.

## Verification and reproduction

Both Release and Debug builds pass. The 74 geometry/physics checks pass. Real Windows key-callback tests exercise full manual, mounted and live takeover controls. Two-second fixed-step comparisons for ownership of all five toys show the owned pose responding to input while the other toy positions match baseline. The complete story rehearsal reaches free exploration in all four raster shading modes, ray tracing and Debug, with every character outside. 58 rendered captures and the complete 120-second video decode also pass. Evidence is retained in [validation](validation/final-validation.json).

The updated presentation passes [separate artifact checks](validation/presentation.json): twelve PDF/deck pages, twelve typeset equation rows, slide bounds, the texture slide, final-slide video embedding, matching embedded/standalone bytes, original recording integrity, full-HD resolution, no audio and complete video decoding.

```powershell
.\tools\build.ps1 -Configuration Release
.\tools\build.ps1 -Configuration Debug
.\tools\check-physics.ps1
python tools/check_showcase_interaction.py
python tools/check_showcase_live.py
python tools/check_escape_rendering.py
python tools/make_showcase.py --capture --video
python tools/build_showcase_docs.py
python tools/check_presentation.py
python tools/package_showcase.py
```

Media/document regeneration uses the packages listed in `tools/showcase-requirements.txt`; report compilation also requires pdfLaTeX (MiKTeX or TeX Live). Rebuild only the LaTeX report with `python tools/build_latex_report.py`. The source uses relative image paths, so retain the logo, diagrams and figures alongside it. The provided artifacts are ready to open.

For an unattended physical rehearsal, launch `bin/Release/HauntedToyRoom.exe --no-intro --story --gameplay-demo --no-raytrace`. Ordinary launch remains interactive.
