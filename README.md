# Open-MedView

This project is a C++ command-line tool for inspecting and anonymizing DICOM images with GDCM.

## Quick Start Commands

The commands below summarize the Windows setup path that was actually validated during development.

### 1. Install base tools

Run these in a normal PowerShell terminal if the tools are not installed yet:

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools
winget install Kitware.CMake
winget install Git.Git
```

In Visual Studio Installer, make sure these components are selected:

- Desktop development with C++
- MSVC v143 build tools
- Windows 10/11 SDK
- C++ CMake tools for Windows

### 2. Open the correct terminal

Use `x64 Native Tools Command Prompt for VS 2022`.

Verify the compiler and CMake:

```bat
cl
cmake --version
```

### 3. Clone and bootstrap vcpkg

The commands below assume you want to place vcpkg under `H:\vcpkg`.

```bat
cd /d H:\vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd /d H:\vcpkg\vcpkg
bootstrap-vcpkg.bat
set VCPKG_ROOT=H:\vcpkg\vcpkg
echo %VCPKG_ROOT%
```

### 4. Install project dependencies

```bat
cd /d %VCPKG_ROOT%
vcpkg install gdcm:x64-windows
```

Optional:

```bat
vcpkg install gdcm[tools]:x64-windows
```

### 5. Configure and build this project

```bat
cd /d H:\GitHub_Local\DicomHeaderMod
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-debug
```

Release build:

```bat
cmake --build --preset vs2022-x64-release
```

### 6. Verify the executable

```bat
H:\GitHub_Local\DicomHeaderMod\build\vs2022-x64\Debug\OpenMedView.exe --help
```

### 7. Inspect a DICOM file

```powershell
.\build\vs2022-x64\Debug\OpenMedView.exe --inspect --input "C:\path\to\sample.dcm"
```

### 8. Run anonymization with the example rules

```powershell
.\build\vs2022-x64\Debug\OpenMedView.exe --anonymize --input "C:\path\to\sample.dcm" --output ".\output\sample-anonymized.dcm" --rules ".\rules\example.json"
```

## Build prerequisites

- Visual Studio 2022 or Build Tools
- CMake 3.21 or newer
- Git
- vcpkg with GDCM installed

## Notes

- Use `x64 Native Tools Command Prompt for VS 2022` when building with MSVC.
- In `cmd`, set the vcpkg root with `set VCPKG_ROOT=H:/vcpkg/vcpkg`.
- The current validated build flow uses `cmake --preset vs2022-x64` and `cmake --build --preset vs2022-x64`.
- The example anonymization rules are stored in `./rules/example.json`.
- The analysis-safe profile is conservative and preserves more analysis context.
- Local test DICOMs are kept under `./testdata/input` and `./testdata/output`.
- Runtime logs are printed to the console and also saved under `./log/` with a timestamped filename.

## MicroDicom Reference Analysis

MicroDicom is a good reference for a lightweight Windows DICOM viewer because it combines a small footprint with a surprisingly broad practical feature set. Its core strengths are fast opening of studies, intuitive interaction, portable packaging, and a workflow that stays focused on image viewing and everyday clinical tasks rather than trying to become a full research platform.

What is worth learning from MicroDicom:

- It treats image viewing as the primary workflow and keeps the UI simple enough for frequent daily use.
- It supports the common tasks users expect immediately: window/level, zoom, pan, scroll, cine playback, measurements, annotations, and tag inspection.
- It covers important multimodal scenarios such as CT, MRI, PET, SPECT, ultrasound, fusion, MPR, and volume rendering without forcing the user into a complicated setup.
- It offers practical utility features like anonymization, DICOM tag editing, export, PACS communication, local study browsing, and portable deployment.
- It is optimized for quick handling of patient studies and CD/USB-style delivery, which is useful if the goal is low-friction viewing on ordinary Windows machines.

Implications for this project:

- Keep the current CLI/core logic as the stable backend, but design the next layer around actual reading workflows, not around isolated command options.
- Add a viewer-oriented front end only after the core image model is stable enough to support scrolling, fusion, and modality-specific presentation.
- Make the UI task-driven: open study, inspect series, compare modalities, measure, segment, anonymize, export.
- Preserve portability and low deployment friction, because a DICOM tool is only useful if clinicians and researchers can run it quickly on a normal workstation.
- Use MicroDicom as a reminder that a good DICOM platform does not have to be huge first; it has to be fast, focused, and reliable on the most common tasks.

## Open-Source Platform Comparison

### OHIF

OHIF is the strongest reference if the end goal is a web-based imaging platform with modern workflow support. It is built around DICOMweb, a plugin-oriented architecture, and a viewer-first product model. Its strengths are fast study loading, browser deployment, configurable task modes, and extensibility for common viewer workflows such as 2D/3D viewing, MPR, MIP, segmentation, RT structures, microscopy, and fusion-oriented use cases.

What to learn from OHIF:

- Separate the core viewer from workflow-specific modes.
- Design for extensibility from the start rather than hard-coding every workflow into one UI.
- Treat DICOMweb and service integration as first-class concerns if web deployment matters.
- Keep the UI organized around clinical tasks, not just around raw data browsing.

What not to copy blindly:

- Do not assume the web is the best first step if your current strength is native Windows tooling.
- Do not overbuild the plugin framework before the image core and workflow model are stable.

### 3D Slicer

3D Slicer is the best reference if the product direction is a desktop research and post-processing workstation. It is strong in multimodal image handling, segmentation, registration, measurement, quantitative analysis, and module-based extensibility. It is closer to a platform than a viewer: users build workflows by combining specialized modules rather than only reading studies.

What to learn from 3D Slicer:

- Keep a stable core data model and let features grow as modules.
- Make segmentation, registration, and quantification first-class workflows, not add-ons.
- Support scripting and automation early if the platform targets research users.
- Expect users to combine modalities and derived objects in one session.

What not to copy blindly:

- Do not start with a huge module ecosystem before the core viewer and processing pipeline are reliable.
- Do not treat every medical imaging task as a plugin problem; some core behaviors must remain native and consistent.

### ITK-SNAP

ITK-SNAP is the best reference for a focused, high-usability tool centered on segmentation and interactive analysis. It is intentionally narrower than OHIF or 3D Slicer, but that narrower scope makes it approachable, fast to learn, and effective for repeated expert use.

What to learn from ITK-SNAP:

- Keep the interaction model simple and predictable.
- Optimize heavily for one or two high-value tasks instead of trying to cover everything.
- Make manual editing, guided segmentation, and navigation feel immediate.
- Preserve a strong balance between automation and user control.

What not to copy blindly:

- Do not make the platform so narrow that it cannot evolve beyond a single core task.
- Do not sacrifice modality breadth if your project goal is a multi-modality post-processing platform.

### Combined Takeaway for This Project

The best product direction for this repository is not to imitate one project exactly, but to combine their strongest ideas:

- From MicroDicom: lightweight, fast, task-focused day-to-day usability.
- From OHIF: extensibility, workflow modes, and clear separation between viewer and features.
- From 3D Slicer: module-based post-processing, multimodal analysis, and a stable core data model.
- From ITK-SNAP: a narrow, efficient interaction model for the highest-frequency tasks.

For this project specifically, the next platform shape should be:

- A shared core library for DICOM loading, modality abstraction, image geometry, and processing.
- A native desktop first UI for browsing, fusion, measurement, segmentation, and anonymization.
- A later web or remote access layer only after the local desktop workflow is stable.
- A workflow-driven structure with modes such as review, compare, fuse, measure, segment, anonymize, and export.

## Design Notes

### 1. Goal

Build a C++ command-line tool based on GDCM that can:

- Scan one file or a directory of DICOM files
- Read and display all tag information, including nested sequences
- Apply batch anonymization rules to selected tags
- Write results to new files or overwrite in-place when explicitly allowed
- Produce a processing report for each file

The focus is anonymization, not raw tag deletion. Some DICOM tags are mandatory or semantically important, so the default behavior should prefer safe replacement, clearing, or structured anonymization rules instead of unconditional removal.

### 2. High-Level Architecture

The application is split into independent layers:

1. CLI layer
	- Parses command-line arguments
	- Selects mode: inspect, anonymize, or dry-run
	- Resolves input and output paths

2. File discovery layer
	- Enumerates files from a single path or a directory
	- Supports recursive traversal
	- Filters non-DICOM files safely

3. DICOM inspection layer
	- Reads metadata with GDCM
	- Expands tags into a uniform in-memory representation
	- Handles sequences recursively

4. Anonymization engine
	- Applies rule-based transformations to the in-memory dataset
	- Supports replacement, clearing, masking, and conditional rules
	- Preserves file validity as much as possible

5. Writer and reporting layer
	- Saves modified datasets to disk
	- Emits a per-file report with changed tags, warnings, and errors

### 3. Suggested Workflow

Input path -> File discovery -> DICOM read -> Tag normalization -> Rule application -> Write output -> Report

For inspect mode, the pipeline stops after tag normalization and prints the dataset.
For anonymize mode, the pipeline continues through rule application and output writing.

### 4. Core Modules

#### 4.1 `cli`

Responsibility:

- Parse arguments
- Validate required options
- Build a `ProcessingConfig`
- Dispatch to inspect or anonymize workflow

#### 4.2 `scanner`

Responsibility:

- Enumerate candidate files
- Detect DICOM files
- Return a stable list of file jobs

#### 4.3 `dicom_reader`

Responsibility:

- Load a DICOM file with GDCM
- Extract all metadata entries
- Build an in-memory dataset tree

#### 4.4 `tag_model`

Responsibility:

- Define the internal representation of a DICOM dataset
- Store tag, VR, VM, value, nesting level, and sequence children

#### 4.5 `rule_engine`

Responsibility:

- Load anonymization rules
- Match rules against tags or conditions
- Apply transformations to a document

#### 4.6 `writer`

Responsibility:

- Serialize the modified document back to a DICOM file
- Preserve essential file structure where possible
- Support output directory mirroring and optional overwrite mode

#### 4.7 `report`

Responsibility:

- Collect per-file status
- Record modified tags, skipped files, and warnings
- Print human-readable console output and optional machine-readable output

### 5. Data Model Design

#### 5.1 `TagKey`

Stores the identity of a DICOM tag.

Fields:

- Group
- Element
- String representation such as `(0010,0010)`

#### 5.2 `TagEntry`

Represents a single metadata element.

Fields:

- `TagKey key`
- `std::string name`
- `std::string vr`
- `std::string vm`
- `std::string value`
- `bool isSequence`
- `int depth`

#### 5.3 `TagNode`

Represents a tree node for nested sequence support.

Fields:

- `TagEntry entry`
- `std::vector<TagNode> children`

#### 5.4 `DicomDocument`

Represents a whole file in memory.

Fields:

- Source file path
- Transfer syntax information
- Root tag nodes
- Processing metadata

### 6. Anonymization Strategy

Recommended actions:

- `REPLACE` - replace the value with a fixed string or generated token
- `CLEAR` - keep the tag but set its value to empty
- `MASK` - preserve format while hiding part of the value
- `DROP` - remove the tag only if the rule explicitly allows it
- `RECURSIVE_REPLACE` - apply the same action inside sequences

Recommended default behavior:

- For patient-identifying fields, prefer replacement or clearing
- For structural or required tags, do not delete by default
- For private tags, allow optional removal if the user enables it

### 7. Configuration Design

Use a rule file instead of only long command-line arguments. This makes batch anonymization easier to maintain.

Possible rule file formats:

- JSON
- YAML
- TOML

Suggested rule fields:

- `tag`: target tag key or pattern
- `action`: replace, clear, mask, drop
- `value`: replacement value when applicable
- `scope`: top-level only or recursive
- `condition`: optional filter based on tag name or VR

Example logical rule definition:

- Replace `(0010,0010)` with `ANONYMIZED`
- Clear `(0010,0020)`
- Mask `(0010,0030)`

### 8. Command-Line Modes

Inspect mode:

- Read files and print all tags
- No file modifications
- Optional export to text or JSON

Anonymize mode:

- Apply rules and write modified output
- Support dry-run
- Support output directory mirroring
- Support overwrite only when explicitly requested

### 9. DICOM Standard Print Format Interpretation

The inspect output should be documented using the official DICOM concepts defined in PS3.5 and the Data Dictionary defined in PS3.6.

#### 9.1 Standard Background

According to DICOM PS3.5, a Data Element is composed of these logical parts:

- Data Element Tag
- Value Representation (VR)
- Value Length (VL)
- Value Field

The official attribute name, standard VR, and Value Multiplicity (VM) for a Standard Data Element are defined by the DICOM Data Dictionary in PS3.6.

#### 9.2 Expected Human-Readable Line Format

The current CLI output follows the common diagnostic style used by GDCM and similar DICOM tools:

`(gggg,eeee) VR [value] # vl,vm name`

Example:

`(0010,0010) PN [ANONYMIZED] # 10,1 Patient's Name`

#### 9.3 Field-by-Field Meaning

Tag:

- Printed as `(gggg,eeee)`
- `gggg` is the Group Number
- `eeee` is the Element Number

VR:

- Printed as a two-letter code such as `PN`, `LO`, `DA`, `UI`, or `SQ`
- This corresponds to the official Value Representation of the Data Element

Value:

- Printed inside brackets such as `[ANONYMIZED]`
- If a tag is present but empty, tools may print `(no value)` or an equivalent placeholder

VL:

- Printed after `#` as the first number, for example `10` in `# 10,1`

VM:

- Printed after the comma in `# vl,vm`, for example `1` in `# 10,1`

Name:

- Printed as the human-readable attribute name such as `Patient's Name`

#### 9.4 Sequence and Nested Printing

When VR is `SQ`, the Value contains a Sequence of Items rather than a plain scalar text value.

The print logic should treat this specially:

- Print the parent sequence tag and name
- Indicate that the element is a sequence
- Traverse Items recursively
- Preserve nesting depth in the output format

#### 9.5 Character Set Considerations

Printed text values are not always a byte-for-byte representation of the stored field.

Important implications:

- The displayed value may be decoded using Specific Character Set `(0008,0005)`
- Character count and byte count may differ for multi-byte encodings
- Person Name `PN` has additional internal structure using `^` and possibly `=` delimiters

### 10. Error Handling

The program should fail gracefully per file, not per entire batch.

Recommended error categories:

- Input file not found
- Not a valid DICOM file
- Read failure
- Rule parse failure
- Write failure
- Unsupported tag transformation

### 11. Suggested C++ Code Style

- Use `std::filesystem` for path handling
- Use RAII for resource management
- Prefer small classes with single responsibility
- Keep business logic out of `main()`
- Use English for all code comments
- Use descriptive names instead of abbreviations

### 12. GDCM Environment Setup

Recommended environment on Windows:

- Visual Studio 2022 with the C++ desktop workload
- CMake
- Git
- vcpkg for dependency management

### 13. Recommended Build Order

1. CLI parsing and file discovery
2. DICOM reading and tag printing
3. Internal data model for tags and sequences
4. Rule loading and anonymization engine
5. File writing and reporting
6. Recursive batch processing and polish

### 14. Minimum Viable Version

The first useful version should support:

- One input file or one directory
- Recursive scanning
- Full tag display
- JSON or text rule file
- Batch anonymization of selected tags
- Output to a separate directory
- Per-file summary report
