# Bliss ZBP / ZBB File Format Specification

> **Product:** Bliss -- sampler/synthesizer plugin by [discoDSP](https://www.discodsp.com/)
> **Version documented:** 3.7.4 (`0x030704`)
> **Date:** 2026-03-01

---

## Table of Contents

1. [Overview](#overview)
2. [File Extensions](#file-extensions)
3. [Container Format](#container-format)
4. [ZBP -- Program/Preset File](#zbp----programpreset-file)
5. [ZBB -- Bank File](#zbb----bank-file)
6. [XML Schema: Program](#xml-schema-program)
7. [XML Schema: Zone](#xml-schema-zone)
8. [Enumerations Reference](#enumerations-reference)
9. [Data Types](#data-types)
10. [Value Mappings](#value-mappings)
11. [Macro System](#macro-system)
12. [Effects Parameters](#effects-parameters)
13. [ConvertWithMoss Integration Notes](#convertwithmoss-integration-notes)
14. [Known Limitations](#known-limitations)

---

## Overview

Bliss is a feature-rich software sampler plugin with multi-zone instruments, dual filters, dual LFOs, amplitude and modulation envelopes, per-zone effects sends, unison, and 8 assignable macros. Each program (preset) may contain up to 128 zones, each mapping audio samples across MIDI key/velocity ranges.

Both file formats are **ZIP archives** containing XML metadata and FLAC-compressed audio samples.

**XML format note:** Bliss uses JUCE's `ValueTree` for serialization. Scalar properties are stored as **XML attributes** on the element, while compound objects (RTPAR, MIDI state, arrays) are **child elements** with their own attributes. This is important for parser implementations.

---

## File Extensions

| Extension | Description          | MIME suggestion            |
|-----------|----------------------|----------------------------|
| `.zbp`    | Single program/preset | `application/x-bliss-program` |
| `.zbb`    | Bank (multi-program)  | `application/x-bliss-bank`    |

---

## Container Format

Both `.zbp` and `.zbb` files are standard ZIP archives (deflate compression).

**Audio samples** within the ZIP are stored as **FLAC** files:
- Licensed builds: 24-bit FLAC
- Demo builds: 16-bit FLAC
- FLAC compression level: 5

**XML metadata** is stored at ZIP compression level 9. Audio entries are stored at ZIP compression level 0 (stored without additional compression, since FLAC is already compressed).

---

## ZBP -- Program/Preset File

### ZIP Contents

```
program.xml                    <- Program metadata + zone parameters (ZIP level 9)
program_000/zone_000.flac      <- Audio for zone 0 (ZIP level 0)
program_000/zone_001.flac      <- Audio for zone 1
program_000/zone_NNN.flac      <- Audio for zone N
```

### Notes

- Zone audio files are named `program_000/zone_XXX.flac` (three-digit zero-padded index).
- A program may contain **0 to 128 zones**.
- If a zone references an external sample file (`path` attribute is set), the FLAC audio entry for that zone may be omitted and the original file on disk is used.
- Zones with destructive edits (e.g. time-stretch, reversal) store a base64-encoded fallback waveform inside the XML under an `edited_waveform` element.

---

## ZBB -- Bank File

### ZIP Contents

```
bank.xml                        <- Bank metadata and state (ZIP level 9)
program_000/zone_000.flac       <- Zone 0 audio for program 0 (ZIP level 0)
program_000/zone_NNN.flac
program_001/zone_000.flac       <- Zone 0 audio for program 1
...
program_127/zone_MMM.flac       <- Up to 128 programs
```

### `bank.xml` Structure

The bank XML has a `<bank>` root element containing two child elements: `<status>` and `<programs>`.

```xml
<?xml version="1.0" encoding="UTF-8"?>

<bank>
  <status interpolation="0"
          oversample="1"
          select_zone_by_midi="0"
          editor_opened="0"
          current_program="0"
          current_zone="0"
          save_samples_plugin="0"/>
  <programs>
    <program version="0x030704" name="Preset 1" ...>
      <!-- Same structure as program.xml (see below) -->
    </program>
    <program version="0x030704" name="Preset 2" ...>
      ...
    </program>
    <!-- up to 128 programs -->
  </programs>
</bank>
```

### `status` Attributes

| Attribute | Type | Description |
|-----------|------|-------------|
| `interpolation` | int | Interpolation quality: 0=Normal, 1=High, 2=Extreme |
| `oversample` | int | Oversampling factor |
| `select_zone_by_midi` | bool | Select zone by incoming MIDI |
| `editor_opened` | bool | Whether sample editor was open |
| `current_program` | int | Currently selected program index |
| `current_zone` | int | Currently selected zone index |
| `save_samples_plugin` | bool | Save samples with plugin instance |

---

## XML Schema: Program

File: `program.xml` (inside ZBP). In a ZBB bank, each `<program>` inside `<programs>` follows the same structure.

All scalar properties are stored as **XML attributes** on the `<program>` element. Compound structures (macros, zones) are child elements.

```xml
<program version="0x030704"
         name="My Preset"
         solo_zone="-1"
         num_zones="4"
         ply_mode="2"
         zone_selection="0.5"
         efx_dft="0"
         efx_dft_pre="0"
         efx_dft_frq="0.4"
         efx_chr="1"
         efx_chr_del="0.1"
         efx_chr_fdb="0.5"
         efx_chr_rat="0.1"
         efx_chr_dep="1.0"
         efx_chr_pha="0.0"
         efx_del="1"
         efx_del_del_l="0.5"
         efx_del_del_r="0.5"
         efx_del_fdb_l="0.5"
         efx_del_fdb_r="0.5"
         efx_del_frq="1.0"
         efx_del_res="0.0"
         efx_del_syn_l="0"
         efx_del_syn_r="0"
         efx_del_cro="0"
         efx_rev="1"
         efx_rev_dam="0.0"
         efx_rev_dec="0.5"
         efx_rev_den="1.0"
         efx_rev_ear="0.5"
         efx_rev_frq="1.0"
         efx_rev_pre="0.0"
         efx_rev_siz="1.0"
         efx_softsat="0"
         efx_softsat_pre="0"
         efx_softsat_drv="0.25"
         efx_bitsreducer="0"
         efx_bitsreducer_pre="0"
         efx_bitsreducer_bits="1.0"
         efx_stereo_rotation="0.0"
         efx_rck="0"
         efx_softclip="0">

  <macros>
    <macro1 name="Cutoff" num_links="1">
      <value value="0.5" sense="0.5"/>
      <links>
        <link1 zone_index="0" param_index="15" min_value="0.0" max_value="1.0"/>
        <!-- more links... -->
      </links>
    </macro1>
    <!-- macro2 through macro8 follow same structure -->
  </macros>

  <zones>
    <!-- See "XML Schema: Zone" section below -->
    <zone ...> ... </zone>
    <zone ...> ... </zone>
  </zones>

</program>
```

### Program Attributes

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `version` | hex int | `0` | Format version, e.g. `0x030704` = 3.7.4 |
| `name` | string | `"---"` | Preset name |
| `solo_zone` | int | `-1` | Soloed zone index (-1 = none) |
| `num_zones` | int | `0` | Number of zones in this program |
| `ply_mode` | int | `2` | Play mode: 0=Mono, 1=Legato, 2=Poly |
| `zone_selection` | float | `0.0` | Zone morphing control (0.0-1.0) |

---

## XML Schema: Zone

Each `<zone>` element within `<zones>` describes one mapped sample layer. Scalar fields are **attributes**; RTPAR fields and MIDI state are **child elements**.

```xml
<zone name="Pad Layer 1"
      path="/path/to/sample.wav"
      num_channels="2"
      num_samples="88200"
      sample_rate="44100"
      mp_num_ticks="4"
      mp_synchro="0"
      mp_gain="0"
      mp_pan="0"
      loop_mode="1"
      loop_start="0"
      loop_end="88199"
      loop_crossfade_len="0.2"
      midi_trigger="0"
      midi_root_key="60"
      midi_fine_tune="0"
      midi_coarse_tune="0"
      midi_keycents="100"
      audio_out="0"
      cue_stop="1"
      cue_white="0"
      num_cues="0"
      res_group="0"
      res_offby="0"
      view_offset="0.0"
      view_length="1.0"
      select_offset="0.0"
      select_length="0.0"
      select_mode="0"
      pitch_mode="0"
      sys_pit_rng="0.0833333"
      stretch_factor="1.0"
      vel_mod="1.0"
      vel_amp="1.0"
      glide_auto="0"
      unison_enable="0"
      unison_voices="4"
      unison_detune="60.0"
      flt1_type="0"
      flt1_kbd_trk="0.0"
      flt1_vel_trk="0.0"
      flt1_boost="1"
      flt2_mode="0"
      flt2_type="0"
      flt2_kbd_trk="0.0"
      flt2_vel_trk="0.0"
      flt2_boost="1"
      amp_env_dest1="0.03571"
      amp_env_dest2="0.03571"
      mod_env_dest1="0.03571"
      mod_env_dest2="0.03571"
      mod_lfo1_type="0"
      mod_lfo1_dest="0.03571"
      mod_lfo1_syn="0"
      mod_lfo2_type="0"
      mod_lfo2_dest="0.03571"
      mod_lfo2_syn="0"
      efx_chr_enable="1"
      efx_del_enable="1"
      efx_rev_enable="1"
      has_destructive_edits="0">

  <!-- MIDI input ranges (child elements with attributes) -->
  <lo_input_range midi_key="36" midi_vel="0" midi_bend="-8192"
                  midi_chanaft="0" midi_polyaft="0">
    <midi_cc val0="0" val1="0" ... val127="0"/>
  </lo_input_range>
  <hi_input_range midi_key="60" midi_vel="127" midi_bend="8192"
                  midi_chanaft="127" midi_polyaft="127">
    <midi_cc val0="127" val1="127" ... val127="127"/>
  </hi_input_range>

  <!-- Cue points (indexed attributes) -->
  <cue_pos val0="0" val1="0" ... val127="0"/>

  <!-- Glide (RTPAR) -->
  <glide value="0.0" sense="0.5"/>

  <!-- Filter 1 (RTPAR children) -->
  <flt1_cut_frq value="1.0" sense="0.5"/>
  <flt1_res_amt value="0.0" sense="0.5"/>
  <flt1_drv_amt value="0.0" sense="0.5"/>

  <!-- Filter 2 (RTPAR children) -->
  <flt2_cut_frq value="1.0" sense="0.5"/>
  <flt2_res_amt value="0.0" sense="0.5"/>
  <flt2_drv_amt value="0.0" sense="0.5"/>

  <!-- Amplitude Envelope ADSR (RTPAR children) -->
  <amp_env_att value="0.0" sense="0.5"/>
  <amp_env_att_shp value="0.5" sense="0.5"/>
  <amp_env_dec value="0.5" sense="0.5"/>
  <amp_env_dec_shp value="0.5" sense="0.5"/>
  <amp_env_sus value="1.0" sense="0.5"/>
  <amp_env_rel value="0.2" sense="0.5"/>
  <amp_env_rel_shp value="0.5" sense="0.5"/>
  <amp_env_amt value="1.0" sense="0.5"/>
  <amp_env_dest1amt value="0.0" sense="0.5"/>
  <amp_env_dest2amt value="0.0" sense="0.5"/>

  <!-- Modulation Envelope ADSR (RTPAR children) -->
  <mod_env_att value="0.0" sense="0.5"/>
  <mod_env_att_shp value="0.5" sense="0.5"/>
  <mod_env_dec value="0.5" sense="0.5"/>
  <mod_env_dec_shp value="0.5" sense="0.5"/>
  <mod_env_sus value="0.5" sense="0.5"/>
  <mod_env_rel value="0.1" sense="0.5"/>
  <mod_env_rel_shp value="0.5" sense="0.5"/>
  <mod_env_amt value="1.0" sense="0.5"/>
  <mod_env_dest1amt value="0.0" sense="0.5"/>
  <mod_env_dest2amt value="0.0" sense="0.5"/>

  <!-- LFO 1 (RTPAR children) -->
  <mod_lfo1_del value="0.0" sense="0.5"/>
  <mod_lfo1_phs value="1.0" sense="0.5"/>
  <mod_lfo1_rat value="0.7" sense="0.5"/>
  <mod_lfo1_amt value="0.0" sense="0.5"/>

  <!-- LFO 2 (RTPAR children) -->
  <mod_lfo2_del value="0.0" sense="0.5"/>
  <mod_lfo2_phs value="1.0" sense="0.5"/>
  <mod_lfo2_rat value="0.7" sense="0.5"/>
  <mod_lfo2_amt value="0.0" sense="0.5"/>

  <!-- Per-zone effect sends (RTPAR children) -->
  <efx_chr_lev value="0.0" sense="0.5"/>
  <efx_del_lev value="0.0" sense="0.5"/>
  <efx_rev_lev value="0.0" sense="0.5"/>

</zone>
```

### Zone Attributes Reference

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | string | `"Untitled"` | Zone name |
| `path` | string | `""` | Original sample file path (empty if embedded) |
| `num_channels` | int | `0` | 1=mono, 2=stereo |
| `num_samples` | int | `0` | Total sample frames |
| `sample_rate` | int | `44100` | Sample rate in Hz |
| `mp_num_ticks` | int | `4` | Beat-sync ticks (0=off) |
| `mp_synchro` | int | `0` | 0=Free, 1=Sync |
| `mp_gain` | int | `0` | Gain offset in dB |
| `mp_pan` | int | `0` | Pan: -100 (L) to +100 (R) |
| `loop_mode` | int | `0` | Loop mode (see enums) |
| `loop_start` | int | `0` | Loop start frame |
| `loop_end` | int | `0` | Loop end frame |
| `loop_crossfade_len` | float | `0.2` | Crossfade length 0.0-1.0 |
| `midi_trigger` | int | `0` | 0=Attack, 1=Release |
| `midi_root_key` | int | `60` | Root key (60=C4) |
| `midi_fine_tune` | int | `0` | Fine tune in cents |
| `midi_coarse_tune` | int | `0` | Coarse tune in semitones |
| `midi_keycents` | int | `100` | Key tracking cents/key |
| `audio_out` | int | `0` | Output routing (see enums) |
| `cue_stop` | int | `1` | Stop at cue: 0=off, 1=on |
| `cue_white` | int | `0` | Cue white key mode |
| `num_cues` | int | `0` | Number of active cue points |
| `res_group` | int | `0` | Resource group (SFZ `group=`) |
| `res_offby` | int | `0` | Off-by group (SFZ `off_by=`) |
| `view_offset` | double | `0.0` | Editor view offset |
| `view_length` | double | `1.0` | Editor view length |
| `select_offset` | double | `0.0` | Selection start |
| `select_length` | double | `0.0` | Selection length |
| `select_mode` | int | `0` | 0=L+R, 1=Left, 2=Right |
| `pitch_mode` | int | `0` | 0=Resample, 1=PitchShift |
| `sys_pit_rng` | float | `0.0833` | Pitch bend range (~200/2400) |
| `stretch_factor` | float | `1.0` | Time-stretch multiplier |
| `vel_mod` | float | `1.0` | Velocity modulation amount |
| `vel_amp` | float | `1.0` | Velocity amplitude scaling (0.0-1.0) |
| `glide_auto` | int | `0` | 0=Free, 1=Auto |
| `unison_enable` | int | `0` | 0=off, 1=on |
| `unison_voices` | int | `4` | Number of unison voices (1-8) |
| `unison_detune` | float | `60.0` | Unison detune amount |
| `flt1_type` | int | `0` | Filter 1 type (see enums) |
| `flt1_kbd_trk` | float | `0.0` | Keyboard tracking (0.0-1.0) |
| `flt1_vel_trk` | float | `0.0` | Velocity tracking (0.0-1.0) |
| `flt1_boost` | int | `1` | Boost mode: 0=off, 1=on |
| `flt2_mode` | int | `0` | 0=Serial, 1=Parallel |
| `flt2_type` | int | `0` | Filter 2 type (see enums) |
| `flt2_kbd_trk` | float | `0.0` | Keyboard tracking |
| `flt2_vel_trk` | float | `0.0` | Velocity tracking |
| `flt2_boost` | int | `1` | Boost mode |
| `amp_env_dest1` | float | `0.03571` | Amp env destination 1 (RTDST float) |
| `amp_env_dest2` | float | `0.03571` | Amp env destination 2 |
| `mod_env_dest1` | float | `0.03571` | Mod env destination 1 |
| `mod_env_dest2` | float | `0.03571` | Mod env destination 2 |
| `mod_lfo1_type` | int | `0` | LFO 1 waveform (see enums) |
| `mod_lfo1_dest` | float | `0.03571` | LFO 1 destination (RTDST float) |
| `mod_lfo1_syn` | int | `0` | LFO 1 host sync: 0=Free, 1=Sync |
| `mod_lfo2_type` | int | `0` | LFO 2 waveform |
| `mod_lfo2_dest` | float | `0.03571` | LFO 2 destination |
| `mod_lfo2_syn` | int | `0` | LFO 2 host sync |
| `efx_chr_enable` | int | `1` | Chorus send enable |
| `efx_del_enable` | int | `1` | Delay send enable |
| `efx_rev_enable` | int | `1` | Reverb send enable |
| `has_destructive_edits` | bool | `false` | Has destructive sample edits |

### Zone Child Elements (RTPAR)

All RTPAR child elements have the format `<name value="..." sense="..."/>`.

| Element | Default value | Default sense | Description |
|---------|---------------|---------------|-------------|
| `glide` | 0.0 | 0.5 | Glide/portamento time |
| `flt1_cut_frq` | 1.0 | 0.5 | Filter 1 cutoff (0.0-1.0) |
| `flt1_res_amt` | 0.0 | 0.5 | Filter 1 resonance |
| `flt1_drv_amt` | 0.0 | 0.5 | Filter 1 drive |
| `flt2_cut_frq` | 1.0 | 0.5 | Filter 2 cutoff |
| `flt2_res_amt` | 0.0 | 0.5 | Filter 2 resonance |
| `flt2_drv_amt` | 0.0 | 0.5 | Filter 2 drive |
| `amp_env_att` | 0.0 | 0.5 | Amp envelope attack |
| `amp_env_att_shp` | 0.5 | 0.5 | Attack shape |
| `amp_env_dec` | 0.5 | 0.5 | Amp envelope decay |
| `amp_env_dec_shp` | 0.5 | 0.5 | Decay shape |
| `amp_env_sus` | 1.0 | 0.5 | Amp envelope sustain |
| `amp_env_rel` | 0.2 | 0.5 | Amp envelope release |
| `amp_env_rel_shp` | 0.5 | 0.5 | Release shape |
| `amp_env_amt` | 1.0 | 0.5 | Amp envelope amount |
| `amp_env_dest1amt` | 0.0 | 0.5 | Dest 1 modulation amount |
| `amp_env_dest2amt` | 0.0 | 0.5 | Dest 2 modulation amount |
| `mod_env_att` | 0.0 | 0.5 | Mod envelope attack |
| `mod_env_att_shp` | 0.5 | 0.5 | Attack shape |
| `mod_env_dec` | 0.5 | 0.5 | Mod envelope decay |
| `mod_env_dec_shp` | 0.5 | 0.5 | Decay shape |
| `mod_env_sus` | 0.5 | 0.5 | Mod envelope sustain |
| `mod_env_rel` | 0.1 | 0.5 | Mod envelope release |
| `mod_env_rel_shp` | 0.5 | 0.5 | Release shape |
| `mod_env_amt` | 1.0 | 0.5 | Mod envelope amount |
| `mod_env_dest1amt` | 0.0 | 0.5 | Dest 1 modulation amount |
| `mod_env_dest2amt` | 0.0 | 0.5 | Dest 2 modulation amount |
| `mod_lfo1_del` | 0.0 | 0.5 | LFO 1 delay |
| `mod_lfo1_phs` | 1.0 | 0.5 | LFO 1 phase |
| `mod_lfo1_rat` | 0.7 | 0.5 | LFO 1 rate |
| `mod_lfo1_amt` | 0.0 | 0.5 | LFO 1 amount |
| `mod_lfo2_del` | 0.0 | 0.5 | LFO 2 delay |
| `mod_lfo2_phs` | 1.0 | 0.5 | LFO 2 phase |
| `mod_lfo2_rat` | 0.7 | 0.5 | LFO 2 rate |
| `mod_lfo2_amt` | 0.0 | 0.5 | LFO 2 amount |
| `efx_chr_lev` | 0.0 | 0.5 | Chorus send level |
| `efx_del_lev` | 0.0 | 0.5 | Delay send level |
| `efx_rev_lev` | 0.0 | 0.5 | Reverb send level |

### MIDI Input Range Child Elements

```xml
<lo_input_range midi_key="0" midi_vel="0" midi_bend="-8192"
                midi_chanaft="0" midi_polyaft="0">
  <midi_cc val0="0" val1="0" ... val127="0"/>
</lo_input_range>
```

| Attribute | Type | lo default | hi default | Description |
|-----------|------|-----------|-----------|-------------|
| `midi_key` | int | 0 | 127 | MIDI key number |
| `midi_vel` | int | 0 | 127 | MIDI velocity |
| `midi_bend` | int | -8192 | 8192 | Pitch bend |
| `midi_chanaft` | int | 0 | 127 | Channel aftertouch |
| `midi_polyaft` | int | 0 | 127 | Polyphonic aftertouch |

The `midi_cc` child element contains `val0` through `val127` attributes for per-CC filtering.

---

## Enumerations Reference

### Play Mode (`ply_mode`)

| Value | Name    |
|-------|---------|
| 0     | Mono    |
| 1     | Legato  |
| 2     | Poly    |

### Loop Mode (`loop_mode`)

| Value | Name          | Notes |
|-------|---------------|-------|
| 0     | OneShot       | No looping; plays once |
| 1     | Forward       | Loops forward between loop_start and loop_end |
| 2     | Bidirectional | Ping-pong loop |
| 3     | Backward      | Plays in reverse |
| 4     | Sustained     | Loops while note is held |
| 5     | Crossfade     | Crossfaded loop using loop_crossfade_len |
| 6     | Static        | Static/fixed position |

### Filter Type (`flt1_type`, `flt2_type`)

| Value | Name       |
|-------|------------|
| 0     | Off        |
| 1     | Lowpass    |
| 2     | Hipass     |
| 3     | Bandpass 1 |
| 4     | Bandpass 2 |
| 5     | Notch      |
| 6     | Peak       |

### Filter Mode (`flt2_mode`)

| Value | Name     | Notes |
|-------|----------|-------|
| 0     | Serial   | Filter 2 follows Filter 1 |
| 1     | Parallel | Filters run in parallel paths |

### LFO Type (`mod_lfo1_type`, `mod_lfo2_type`)

| Value | Name     |
|-------|----------|
| 0     | Sine     |
| 1     | Triangle |
| 2     | Saw      |
| 3     | Square   |
| 4     | Random   |
| 5     | Wander   |

### Modulation Destination (`mod_lfoN_dest`, `amp_env_destN`, `mod_env_destN`)

> **Stored as a normalized float**, not an integer. See the RTDST section in Data Types for the encoding formula and full lookup table.

| Index | Name              | Short | Stored float |
|-------|-------------------|-------|--------------|
| 0     | None              | --    | ~ 0.03571    |
| 1     | Amp               | Amp   | ~ 0.10714    |
| 2     | Pitch             | Pitch | ~ 0.17857    |
| 3     | Filter 1 Freq     | F1 FQ | ~ 0.25000    |
| 4     | Filter 1 Reso     | F1 RS | ~ 0.32143    |
| 5     | Filter 1 Drive    | F1 DR | ~ 0.39286    |
| 6     | Filter 2 Freq     | F2 FQ | ~ 0.46429    |
| 7     | Filter 2 Reso     | F2 RS | ~ 0.53571    |
| 8     | Filter 2 Drive    | F2 DR | ~ 0.60714    |
| 9     | LFO 1 Rate        | L1 RT | ~ 0.67857    |
| 10    | LFO 2 Rate        | L2 RT | ~ 0.75000    |
| 11    | Loop Start        | LoopS | ~ 0.82143    |
| 12    | Loop End          | LoopE | ~ 0.89286    |
| 13    | Pan               | Pan   | ~ 0.96429    |

### MIDI Trigger (`midi_trigger`)

| Value | Name    |
|-------|---------|
| 0     | Attack  |
| 1     | Release |

### Audio Output (`audio_out`)

| Value | Name  |
|-------|-------|
| 0     | Main  |
| 1     | Out A |
| 2     | Out B |
| 3     | Out C |
| 4     | Out D |

### Pitch Mode (`pitch_mode`)

| Value | Name       |
|-------|------------|
| 0     | Resample   |
| 1     | PitchShift |

### Glide Mode (`glide_auto`)

| Value | Name |
|-------|------|
| 0     | Free |
| 1     | Auto |

### Interpolation (bank status)

| Value | Name    |
|-------|---------|
| 0     | Normal  |
| 1     | High    |
| 2     | Extreme |

---

## Data Types

### RTPAR -- Real-Time Parameter

Used for modulatable parameters. Both fields are stored as **attributes** on a child element:

```xml
<flt1_cut_frq value="0.5" sense="0.5"/>
```

| Attribute | Type | Description |
|-----------|------|-------------|
| `value` | float | Parameter value, normalized 0.0-1.0 unless documented otherwise |
| `sense` | float | Morph sensitivity; 0.5 = neutral center |

### RTDST -- Modulation Destination

Stored as a **normalized float** attribute, not an integer. The enum index is mapped to a float using a slot-center encoding with `Count = 14` slots (None through Pan):

```
float = (index + 0.5) / 14
```

| Destination | Index | Stored float value |
|-------------|-------|--------------------|
| None        | 0     | ~ 0.03571          |
| Amp         | 1     | ~ 0.10714          |
| Pitch       | 2     | ~ 0.17857          |
| Filter 1 Freq | 3   | ~ 0.25000          |
| Filter 1 Reso | 4   | ~ 0.32143          |
| Filter 1 Drive | 5  | ~ 0.39286          |
| Filter 2 Freq | 6   | ~ 0.46429          |
| Filter 2 Reso | 7   | ~ 0.53571          |
| Filter 2 Drive | 8  | ~ 0.60714          |
| LFO 1 Rate  | 9     | ~ 0.67857          |
| LFO 2 Rate  | 10    | ~ 0.75000          |
| Loop Start  | 11    | ~ 0.82143          |
| Loop End    | 12    | ~ 0.89286          |
| Pan         | 13    | ~ 0.96429          |

To decode a stored float back to an index: `index = floor(value * 14)`.

```xml
<zone amp_env_dest1="0.25" ...>   <!-- Filter 1 Frequency -->
```

This applies to `amp_env_dest1`, `amp_env_dest2`, `mod_env_dest1`, `mod_env_dest2`, `mod_lfo1_dest`, and `mod_lfo2_dest`.

### Modulation Amount (`amp_env_dest1amt`, `amp_env_dest2amt`, `mod_env_dest1amt`, `mod_env_dest2amt`)

These RTPAR fields are **center-biased** amount knobs. The stored `value` (0.0-1.0) is normalized as:

- **0.5** = no modulation (center / default)
- **< 0.5** = negative modulation depth
- **> 0.5** = positive modulation depth

The engine converts the stored value to a bipolar or unipolar working value depending on the destination:

| Destination | Conversion | Working range | Effective depth |
|-------------|-----------|---------------|-----------------|
| Pitch       | `(value - 0.5)` then `* 4.0` | +-2.0 | **+-2 octaves** at full swing |
| Filter 1/2 Freq | `(value - 0.5)` then `* 4.0` | +-2.0 | Full filter sweep |
| Filter 1/2 Reso/Drive | `(value - 0.5)` then `* 4.0` | +-2.0 | Full range |
| LFO 1/2 Rate | `(value - 0.5)` then `* 4.0` | +-2.0 | Full rate range |
| Loop Start/End | `(value - 0.5)` then `* 4.0` | +-2.0 | Full sample range |
| Pan         | `(value - 0.5)` then `* 4.0` | +-2.0 | Full pan range |
| Amp         | `value * 0.5` | 0.0 to 0.5 | Additive gain (unipolar) |

---

## Value Mappings

### Filter Cutoff Frequency

The normalized `value` attribute (0.0-1.0) is converted to Hz with a **quadratic** mapping:

```
hz = 20 + (x^2) * (22050 - 20)
```

To reconstruct `x` from a frequency in Hz:

```
x = sqrt((hz - 20) / (22050 - 20))
```

Range: 20 Hz (x=0) to 22050 Hz (x=1).

### Envelope Time (Attack, Decay, Release)

The normalized `value` attribute (0.0-1.0) is converted to seconds with a **quartic** mapping with a 1 ms minimum floor:

```
seconds = 0.001 + x^4 * (16.0 - 0.001)
        ~ 0.001 + x^4 * 15.999
```

Maximum envelope time is **16 seconds** (x=1). Minimum is **1 ms** (x=0). To reconstruct `x`:

```
x = ((seconds - 0.001) / 15.999) ^ (1/4)
```

Range: 0.001 s (x=0) to 16 s (x=1).

### Envelope Shape (Attack, Decay, Release shape parameters)

Each envelope stage has a `_shp` RTPAR that controls the **curvature** by specifying the y-value at the midpoint of the stage.

**For Attack** (rising from 0 to 1):

| Value | Curve shape |
|-------|-------------|
| 0.0   | Concave -- slow start, fast end (exponential) |
| 0.5   | Linear (default) |
| 1.0   | Convex -- fast start, slow end (logarithmic) |

**For Decay and Release** (falling from 1 to 0):

| Value | Curve shape |
|-------|-------------|
| 0.0   | Convex -- fast drop, slow tail |
| 0.5   | Linear (default) |
| 1.0   | Concave -- slow drop, fast tail |

The shape parameter represents the value at the temporal midpoint of the stage. When `shape = 0.5`, the envelope is linear. Values away from 0.5 produce exponential curves. The interpretation is reversed between attack (rising) and decay/release (falling) stages.

### Velocity Scaling

```
velocityFloat = midiVelocity / 127.0
velocityScale = 1.0 - (1.0 - velocityFloat) * vel_amp
ampGain = pc_gain * (velocityScale ^ 2)
```

Where `pc_gain = 10^(mp_gain / 20)` (dB to linear).

---

## Macro System

A program has 8 macros. Each macro is a child element within `<macros>`:

```xml
<macro1 name="Cutoff" num_links="1">
  <value value="0.5" sense="0.5"/>
  <links>
    <link1 zone_index="0" param_index="15" min_value="0.0" max_value="1.0"/>
  </links>
</macro1>
```

| Attribute | Type | Description |
|-----------|------|-------------|
| `name` | string | Macro display name (max 64 chars) |
| `num_links` | int | Number of parameter links |

### Link Attributes

| Attribute | Type | Description |
|-----------|------|-------------|
| `zone_index` | int | Zone to control (-1 = all zones) |
| `param_index` | int | Internal parameter index (opaque integer) |
| `min_value` | float | Macro value 0.0 maps to this parameter value |
| `max_value` | float | Macro value 1.0 maps to this parameter value |

Macros are exposed as MIDI CC mappings via program options (CC indices 66-73 for macros 1-8).

---

## Effects Parameters

All effects are program-level attributes on the `<program>` element. Each zone can individually enable/disable the effects bus sends with `efx_chr_enable`, `efx_del_enable`, `efx_rev_enable`.

### Chorus

| Attribute      | Type  | Default | Description            |
|----------------|-------|---------|------------------------|
| `efx_chr`      | int   | 1       | Enable (0=off, 1=on)  |
| `efx_chr_del`  | float | 0.1     | Delay time            |
| `efx_chr_fdb`  | float | 0.5     | Feedback               |
| `efx_chr_rat`  | float | 0.1     | Rate                   |
| `efx_chr_dep`  | float | 1.0     | Depth                  |
| `efx_chr_pha`  | float | 0.0     | Phase                  |

### Delay

| Attribute        | Type  | Default | Description              |
|------------------|-------|---------|--------------------------|
| `efx_del`        | int   | 1       | Enable                  |
| `efx_del_del_l`  | float | 0.5     | Left delay time         |
| `efx_del_del_r`  | float | 0.5     | Right delay time        |
| `efx_del_fdb_l`  | float | 0.5     | Left feedback           |
| `efx_del_fdb_r`  | float | 0.5     | Right feedback          |
| `efx_del_frq`    | float | 1.0     | Filter frequency        |
| `efx_del_res`    | float | 0.0     | Filter resonance        |
| `efx_del_syn_l`  | int   | 0       | Sync left (0=Free, 1=Sync) |
| `efx_del_syn_r`  | int   | 0       | Sync right              |
| `efx_del_cro`    | int   | 0       | Cross-feedback          |

### Reverb

| Attribute       | Type  | Default | Description   |
|-----------------|-------|---------|---------------|
| `efx_rev`       | int   | 1       | Enable       |
| `efx_rev_dam`   | float | 0.0     | Damping      |
| `efx_rev_dec`   | float | 0.5     | Decay        |
| `efx_rev_den`   | float | 1.0     | Density      |
| `efx_rev_ear`   | float | 0.5     | Early Ref.   |
| `efx_rev_frq`   | float | 1.0     | Filter Freq  |
| `efx_rev_pre`   | float | 0.0     | Pre-delay    |
| `efx_rev_siz`   | float | 1.0     | Room Size    |

### Additional Effects

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `efx_dft` | int | 0 | Draft/Hi-Fi enable |
| `efx_dft_pre` | int | 0 | Pre/post (0=Pre, 1=Post) |
| `efx_dft_frq` | float | 0.4 | Draft frequency |
| `efx_softsat` | int | 0 | Soft saturation enable |
| `efx_softsat_pre` | int | 0 | Pre/post |
| `efx_softsat_drv` | float | 0.25 | Drive amount |
| `efx_bitsreducer` | int | 0 | Bit crusher enable |
| `efx_bitsreducer_pre` | int | 0 | Pre/post |
| `efx_bitsreducer_bits` | float | 1.0 | Bit reduction |
| `efx_stereo_rotation` | float | 0.0 | Stereo rotation |
| `efx_rck` | int | 0 | Rack effect enable |
| `efx_softclip` | int | 0 | Soft clip enable |

---

## ConvertWithMoss Integration Notes

### Suggested Format Identifier

- **Detector name:** `BlissPreset` / `BlissBank`
- **File extensions:** `zbp`, `zbb`
- **Detection method:** Open as ZIP, check for `program.xml` (ZBP) or `bank.xml` (ZBB) at root of archive.

### Mapping to ConvertWithMoss IMultisampleSource

| Bliss concept         | ConvertWithMoss mapping suggestion                          |
|-----------------------|-------------------------------------------------------------|
| Program `name` attr   | `IMultisampleSource.name`                                   |
| Zone `name` attr      | `ISampleZone.name`                                         |
| `midi_root_key`       | `ISampleZone.keyRoot`                                       |
| `lo/hi_input_range.midi_key` | `ISampleZone.keyLow` / `keyHigh` |
| `lo/hi_input_range.midi_vel` | `ISampleZone.velocityLow` / `velocityHigh` |
| `mp_gain`             | `ISampleZone.gain` (in dB)                                  |
| `mp_pan`              | `ISampleZone.panorama` (normalize -100..+100 to -1..+1)      |
| `loop_mode` (0=off, 1+=loop) | `ISampleZone.loops[]`                            |
| `loop_start` / `loop_end` | `ISampleLoop.start` / `end`                           |
| `loop_crossfade_len`  | `ISampleLoop.crossfade`                                     |
| `midi_fine_tune` + `midi_coarse_tune` | `ISampleZone.tune` (combine cents + semitones) |
| `amp_env_*`           | `ISampleZone.amplitudeEnvelope`                             |
| `flt1_type`, `flt1_cut_frq`, `flt1_res_amt` | `ISampleZone.filter`                |
| Embedded FLAC samples | Read with standard JFLAC/FLAC decoder; 16 or 24-bit       |

### Reading ZBP

```
1. Open .zbp as ZIP archive
2. Read and parse program.xml (UTF-8 XML, JUCE ValueTree format)
3. Read program attributes (name, version, effects, etc.)
4. For each <zone> element within <zones>:
   a. Read zone attributes for scalar values
   b. Read RTPAR child elements for value/sense pairs
   c. Read lo_input_range / hi_input_range for MIDI mapping
   d. Open program_000/zone_NNN.flac from ZIP
   e. Decode FLAC -> PCM float data
5. Map to IMultisampleSource
```

### Reading ZBB

```
1. Open .zbb as ZIP archive
2. Read and parse bank.xml (UTF-8 XML)
3. Read <status> attributes for bank state
4. For each <program> within <programs>:
   a. Parse program attributes and zones (same as program.xml)
   b. Open program_NNN/zone_MMM.flac from ZIP
5. Yield one IMultisampleSource per program
```

### Writing ZBP (Export to Bliss)

```
1. Create in-memory ZIP
2. Build a JUCE ValueTree (or equivalent attribute-based XML):
   - Program attributes on <program> element
   - Zone attributes on <zone> elements
   - RTPAR as child elements with value/sense attributes
3. Serialize to XML string
4. For each zone:
   a. Encode PCM samples as FLAC (recommend 24-bit, compression=5)
   b. Add to ZIP as program_000/zone_NNN.flac at ZIP level 0
5. Add program.xml to ZIP at level 9
6. Write ZIP -> .zbp file
```

### Version Header

Always write the version attribute on the `<program>` root element:

```xml
<program version="0x030704" ...>
```

The format `0xAABBCC` encodes Major (AA), Minor (BB), Patch (CC) as hex pairs.

---

## Known Limitations

1. **Macro parameter indices** (`param_index` in links) are internal opaque integers determined by Bliss's parameter ordering. A complete mapping table is not published.

2. **Destructive edits / base64 waveform fallback:** When `has_destructive_edits` is `true` (or `1`), the zone may contain an `edited_waveform` element with base64-encoded raw PCM data as a fallback.

3. **Morph/Sense values:** The `sense` attribute inside RTPAR elements is specific to Bliss's morph system and has no standard equivalent in SFZ/SF2/Kontakt. It can be safely ignored on import.

4. **Per-zone output routing:** `audio_out` maps zones to up to 5 output buses. Default to main/stereo output.

5. **External sample references:** If `path` is set and no FLAC is embedded, the converter needs access to the referenced file on disk.

6. **FLAC bit depth:** On demo/unlicensed builds, samples are stored as 16-bit; on licensed builds, 24-bit. Always check the FLAC file header for actual bit depth.

---
