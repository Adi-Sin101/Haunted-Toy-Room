# Haunted Toy Room: The Midnight Mission

Adiba Tahsin | Roll 2107031 | CSE-4102

## Showcase files

| File | Purpose |
| --- | --- |
| [Project-Report.pdf](Project-Report.pdf) | Finished illustrated academic report: 31-page LaTeX report with one combined cover, four chapters, references and object/material summaries |
| [Project-Report.tex](Project-Report.tex) | Editable LaTeX report source |
| [Comprehensive-Implementation-Notes.md](Comprehensive-Implementation-Notes.md) | Complete theory and implementation notes, with every scene node and material |
| [Requirements-Coverage.md](Requirements-Coverage.md) | Teacher requirement mapping to implementations and demonstration evidence |
| [Project-Presentation.pptx](Project-Presentation.pptx) | Editable ten-slide haunted-house deck with speaker notes and the embedded demo on slide 2 |
| [Project-Presentation.pdf](Project-Presentation.pdf) | Portable slide copy |
| [Project-Demo.mp4](Project-Demo.mp4) | Two-minute, 24 fps demonstration recorded from the application |
| [Demonstration-Study-Guide.pdf](Demonstration-Study-Guide.pdf) | Printable narration, rehearsal sequence, viva answers and parameter-change practice |
| [Demonstration-Study-Guide.md](Demonstration-Study-Guide.md) | Searchable/editable learning guide |
| [Submission-Package.zip](Submission-Package.zip) | Runnable application, source, documents, media and validation evidence |

The video has explanatory captions; its timed narration is supplied for the presenter to explain live. The standalone MP4 accompanies the embedded copy.

## Run and demonstrate

From the project folder:

```powershell
.\Run-Showcase.ps1
.\Run-Showcase.ps1 -Mode Manual -Raster
.\Run-Showcase.ps1 -Mode Mounted -Raster
```

The default launch starts Penny's arrival, followed by the mission. Y skips the arrival. During the mission, select a toy with 1-5 or click a character to take live control while the others continue. W/S moves, A/D turns, Shift runs and Space stops. Press 0 to release that actor. N switches the entire scene to full manual mode or resumes playback. Shift+N restarts everything. Penny can be clicked and driven after arrival. Selecting a mounted rider or horse detaches the rider for independent control; remounting intentionally controls the connected pair.

F2 selects raster rendering and cycles Flat/Gouraud/Phong/Blinn. F4 switches analytic ray tracing. F5/F6/F7 toggle illumination terms. F3 toggles textures. F focuses an object; V lists its parts. Tab enables transformations. O enables haunted ambience. H expands the control guide; G hides the interface. Detailed controls are in [the reference](../13-controls.md).

## What is documented

The report contains 19 numbered figure groups, 11 tables and 28 numbered equation groups in 31 pages. It explains all five primitive families, the scene hierarchy, transformations, camera/projection, directional/point/spot lights, illumination equations, four raster shading models, 21 surface maps plus the white fallback, lamp shadow mapping, BVH/analytic ray tracing, animation, collision and independent input ownership. The report appendices summarise the object families and representative materials; the comprehensive implementation notes and CSV exports record all 699 scene nodes and every material in detail. Actual exports are in [inventory](inventory/objects.csv); full-resolution illustrations are in `figures/`, `diagrams/` and `inventory/textures/`.

## Verification and reproduction

Both Release and Debug builds pass. The 36 geometry/physics checks pass. Real Windows key-callback tests exercise full manual, mounted and live takeover controls. All five live-owned toy replays reach The End without resetting the owned toy, while other toys return home. A matched route comparison confirms the other actors continue unchanged when Woody is owned. The full arrival/story replay, 42 rendered captures and complete 120-second video decode also pass. Evidence is retained in [validation](validation/final-validation.json).

```powershell
.\tools\build.ps1 -Configuration Release
.\tools\build.ps1 -Configuration Debug
.\tools\check-physics.ps1
python tools/check_showcase_interaction.py
python tools/check_showcase_live.py
python tools/make_showcase.py --capture --video
python tools/build_showcase_docs.py
python tools/package_showcase.py
```

Media/document regeneration uses the packages listed in `tools/showcase-requirements.txt`; report compilation also requires pdfLaTeX (MiKTeX or TeX Live). Rebuild only the LaTeX report with `python tools/build_latex_report.py`. The source uses relative image paths, so retain the logo, diagrams and figures alongside it. The provided artifacts are ready to open.
