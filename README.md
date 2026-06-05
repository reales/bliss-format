# Bliss ZBP / ZBB Format Specification

Developer resources for reading and writing **Bliss ZBP** (preset) and **ZBB** (bank) files.

[Bliss](https://www.discodsp.com/bliss/) is a sampler/synthesizer plugin by [discoDSP](https://www.discodsp.com/).

## Files

| File | Description |
|------|-------------|
| [Bliss_ZBP_ZBB_Format_Specification.md](Bliss_ZBP_ZBB_Format_Specification.md) | Full format specification: ZIP container layout, XML schema, enumerations, value mappings, and ConvertWithMoss integration notes |
| [bliss_zbp_parser.cpp](bliss_zbp_parser.cpp) | Self-contained C++17 reference parser -- reads and dumps ZBP/ZBB files |

## Quick Start

### Build the reference parser

```bash
g++ -std=c++17 -O2 -o bliss_zbp_parser bliss_zbp_parser.cpp
```

Requires a C++17 compiler and `unzip` on PATH (macOS/Linux). No other dependencies.

### Parse a preset

```bash
./bliss_zbp_parser MyPreset.zbp
```

### Parse a bank

```bash
./bliss_zbp_parser MyBank.zbb
```

### Example output (ZBP)

```
Bliss ZBP/ZBB Parser -- Reference Implementation
File: Piano.zbp

-- Program ---------------------------------------------
  Name:     "Grand Piano"
  Version:  3.7.4 (0x030704)
  Play mode: Poly
  Zones:    4
  Zone 0: "Piano Soft"
    Key range:  C2 (36) - C6 (84)   Root: C4 (60)
    Vel range:  0 - 64
    Tune:       coarse=0 st  fine=0 ct
    Audio:      2 ch, 132300 frames, 44100 Hz
    Loop:       Forward  start=44100  end=132299  xfade=0.20
    Gain:       0 dB   Pan: 0
    Filter 1:   Lowpass  cutoff=1000 Hz  reso=25.0%
    Amp Env:    A=0.010s D=0.500s S=80% R=0.300s
    Env shapes: A=0.50 D=0.50 R=0.50  (0=concave 0.5=linear 1=convex)
    Velocity:   amp=1.00  mod=1.00
  ...

-- Embedded samples ------------------------------------
  program_000/zone_000.flac
  program_000/zone_001.flac
  program_000/zone_002.flac
  program_000/zone_003.flac

Done.
```

### Example output (ZBB)

```
Bliss ZBP/ZBB Parser -- Reference Implementation
File: MyBank.zbb
Interpolation: Normal
Current program: 0

-- Program 0 -------------------------------------------
  Name:     "Grand Piano"
  Version:  3.7.4 (0x030704)
  Play mode: Poly
  Zones:    4
  Zone 0: "Piano Soft"
    Key range:  C2 (36) - C6 (84)   Root: C4 (60)
    ...

-- Program 1 -------------------------------------------
  Name:     "Pad Strings"
  Version:  3.7.4 (0x030704)
  Play mode: Poly
  Zones:    2
  ...

Programs found: 128

Done.
```

## Format Overview

Both `.zbp` and `.zbb` files are **ZIP archives** containing:

- **XML metadata** -- program parameters, zone mappings, effects, macros
- **FLAC audio** -- 16-bit (demo) or 24-bit (licensed) samples at compression level 5

### ZBP layout (single preset)

```
program.xml
program_000/zone_000.flac
program_000/zone_001.flac
...
```

### ZBB layout (bank with multiple programs)

```
bank.xml
program_000/program.xml
program_000/zone_000.flac
program_001/program.xml
program_001/zone_000.flac
...
program_127/program.xml
```

## Key Value Mappings

The reference parser includes conversion helpers for all non-linear mappings:

| Parameter | Formula | Range |
|-----------|---------|-------|
| Filter cutoff | `hz = 20 + x^2 * (22050 - 20)` | 20 Hz - 22050 Hz |
| Envelope time | `sec = 0.001 + x^4 * 15.999` | 1 ms - 16 seconds |
| Mod destination | integer index (3.13+); legacy `index = floor(value * 14)` | 0-13 (append-only) |
| Version hex | `0xAABBCC` -> `A.B.C` | e.g. `0x030704` -> `3.7.4` |

See the [full specification](Bliss_ZBP_ZBB_Format_Specification.md) for complete details.

## Integration

The parser and specification are designed as a reference for building format support in tools like [ConvertWithMoss](https://github.com/git-moss/ConvertWithMoss). Key integration points:

- **Detection**: open as ZIP, check for `program.xml` (ZBP) or `bank.xml` (ZBB)
- **Zones**: each `<zone>` maps to a sample layer with key/velocity ranges
- **Samples**: FLAC files at `program_NNN/zone_MMM.flac`
- **Envelopes**: ADSR with quartic time mapping and configurable shape curves

## License

Bliss is a commercial product by [discoDSP](https://www.discodsp.com/).
This documentation and reference parser are provided for interoperability purposes.
