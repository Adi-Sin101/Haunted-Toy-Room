"""Create a reviewable submission archive without build caches or reference inputs."""
from pathlib import Path
import hashlib
import json
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/showcase"


def package():
    documents = ["START-HERE.md", "Project-Report.pdf", "Project-Report.tex", "Comprehensive-Implementation-Notes.md", "Requirements-Coverage.md", "KUET-LOGO.png", "Project-Report.md",
                 "Project-Presentation.pptx", "Project-Presentation.pdf", "Project-Demo.mp4", "Project-Demo.srt",
                 "Demonstration-Study-Guide.pdf", "Demonstration-Study-Guide.docx", "Demonstration-Study-Guide.md"]
    dest = OUT / "Submission-Package.zip"
    entries = {}
    for name in documents:
        entries["Showcase/" + name] = OUT / name
    for folder in ["figures", "diagrams", "inventory", "validation"]:
        for file in (OUT / folder).rglob("*"):
            if (folder != "validation" or file.suffix.lower() in [".json", ".log"]) and file.is_file() and file.name!="package.json" and not file.name.startswith("draft-") and "package-runtime" not in file.parts and file.suffix.lower() in [".png", ".csv", ".json", ".log"]:
                entries["Showcase/" + file.relative_to(OUT).as_posix()] = file
    for folder in ["src", "shaders", "assets", "Libraries", "tests", "tools"]:
        for file in (ROOT / folder).rglob("*"):
            if file.is_file() and "__pycache__" not in file.parts and file.suffix.lower() != ".pyc":
                entries["Project/" + file.relative_to(ROOT).as_posix()] = file
    for file in ROOT.glob("*"):
        if file.is_file() and file.name in ["README.md", "Run-Showcase.ps1", "HauntedToyRoom.slnx", "HauntedToyRoom.vcxproj", "HauntedToyRoom.vcxproj.filters"]:
            entries["Project/" + file.name] = file
    for file in (ROOT / "docs").glob("*.md"):
        if file.name not in ["project-context.md", "implementation-plan.md"]:
            entries["Project/docs/" + file.name] = file
    for file in (ROOT / "docs/images").glob("*"):
        if file.is_file(): entries["Project/docs/images/" + file.name] = file
    for file in (ROOT / "bin/Release").rglob("*"):
        if file.is_file() and file.suffix.lower() != ".pdb":
            entries["Project/bin/Release/" + file.relative_to(ROOT / "bin/Release").as_posix()] = file
    with zipfile.ZipFile(dest, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, file in sorted(entries.items()):
            assert file.is_file(), file
            archive.write(file, name)
        archive.writestr("OPEN-ME.txt", "Open Showcase/START-HERE.md for the documents and demo.\nRun Project/Run-Showcase.ps1 for the application.\nSelect a toy for live takeover; 0 releases it; N switches full manual/story mode.\n")
        archive.writestr("Project/docs/showcase/START-HERE.md", "# Finished showcase materials\n\nOpen [the showcase guide](../../../Showcase/START-HERE.md) for the report, slides, video and study guide.\n")
    with zipfile.ZipFile(dest) as archive:
        assert archive.testzip() is None
        assert "Project/bin/Release/HauntedToyRoom.exe" in archive.namelist()
        assert "Showcase/Project-Demo.mp4" in archive.namelist()
    manifest = {"archive": dest.name, "files": len(entries)+2, "bytes": dest.stat().st_size,
                "sha256": hashlib.sha256(dest.read_bytes()).hexdigest(), "integrity": "passed"}
    (OUT / "validation/package.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    package()
