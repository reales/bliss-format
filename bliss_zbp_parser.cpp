// bliss_zbp_parser.cpp -- Minimal reference parser for Bliss ZBP/ZBB files
//
// Copyright (c) discoDSP. Public domain reference implementation.
//
// Dependencies: a C++17 compiler and `unzip` on PATH (macOS/Linux).
// This file is a self-contained example. It uses a tiny XML pull-parser
// and shells out to `unzip` for ZIP reading. Replace with your preferred
// ZIP and XML libraries as needed.
//
// Build:
//   g++ -std=c++17 -O2 -o bliss_zbp_parser bliss_zbp_parser.cpp
//
// Usage:
//   ./bliss_zbp_parser <file.zbp|file.zbb>
//
// Note: Bliss uses JUCE ValueTree serialization, which stores scalar
// properties as XML attributes and compound objects as child elements.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

// --- Lightweight XML helpers (no dependency) ---------------------------------
// This is intentionally minimal. For production use, replace with pugixml,
// tinyxml2, or your framework's XML parser.

struct XmlNode {
    std::string tag;
    std::string text;
    std::map<std::string, std::string> attributes;
    std::vector<XmlNode> children;

    const XmlNode* child(const std::string& name) const {
        for (auto& c : children)
            if (c.tag == name) return &c;
        return nullptr;
    }

    // Get attribute value (JUCE ValueTree stores properties as attributes)
    std::string attr(const std::string& name, const std::string& def = "") const {
        auto it = attributes.find(name);
        return (it != attributes.end()) ? it->second : def;
    }

    int attrInt(const std::string& name, int def = 0) const {
        auto it = attributes.find(name);
        return (it != attributes.end()) ? std::atoi(it->second.c_str()) : def;
    }

    float attrFloat(const std::string& name, float def = 0.0f) const {
        auto it = attributes.find(name);
        return (it != attributes.end()) ? std::strtof(it->second.c_str(), nullptr) : def;
    }

    double attrDouble(const std::string& name, double def = 0.0) const {
        auto it = attributes.find(name);
        return (it != attributes.end()) ? std::strtod(it->second.c_str(), nullptr) : def;
    }
};

// Minimal recursive-descent XML parser (handles the subset Bliss uses)
static std::string xmlTrim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool parseXml(const std::string& xml, size_t& pos, XmlNode& node);

static bool parseXml(const std::string& xml, size_t& pos, XmlNode& node) {
    while (pos < xml.size()) {
        size_t lt = xml.find('<', pos);
        if (lt == std::string::npos) break;

        // skip comments
        if (xml.compare(lt, 4, "<!--") == 0) {
            size_t end = xml.find("-->", lt + 4);
            if (end == std::string::npos) break;
            pos = end + 3;
            continue;
        }

        // closing tag
        if (xml[lt + 1] == '/') {
            pos = xml.find('>', lt) + 1;
            return true;
        }

        // opening tag
        size_t gt = xml.find('>', lt);
        if (gt == std::string::npos) break;

        std::string tagContent = xml.substr(lt + 1, gt - lt - 1);
        bool selfClosing = (!tagContent.empty() && tagContent.back() == '/');
        if (selfClosing) tagContent.pop_back();

        XmlNode child;
        // parse tag name and attributes
        size_t sp = tagContent.find_first_of(" \t\r\n");
        if (sp == std::string::npos) {
            child.tag = tagContent;
        } else {
            child.tag = tagContent.substr(0, sp);
            std::string attrStr = tagContent.substr(sp);
            // simple attribute parsing: key="value"
            size_t ap = 0;
            while (ap < attrStr.size()) {
                size_t eq = attrStr.find('=', ap);
                if (eq == std::string::npos) break;
                std::string key = xmlTrim(attrStr.substr(ap, eq - ap));
                size_t q1 = attrStr.find('"', eq + 1);
                if (q1 == std::string::npos) break;
                size_t q2 = attrStr.find('"', q1 + 1);
                if (q2 == std::string::npos) break;
                child.attributes[key] = attrStr.substr(q1 + 1, q2 - q1 - 1);
                ap = q2 + 1;
            }
        }

        pos = gt + 1;

        if (!selfClosing) {
            // collect text content and child elements
            while (pos < xml.size()) {
                size_t nextLt = xml.find('<', pos);
                if (nextLt == std::string::npos) break;

                // accumulate text before next tag
                std::string textBefore = xmlTrim(xml.substr(pos, nextLt - pos));
                if (!textBefore.empty())
                    child.text += textBefore;

                // check for closing tag
                if (xml[nextLt + 1] == '/') {
                    pos = xml.find('>', nextLt) + 1;
                    break;
                }

                // skip comments
                if (xml.compare(nextLt, 4, "<!--") == 0) {
                    size_t end = xml.find("-->", nextLt + 4);
                    if (end == std::string::npos) { pos = xml.size(); break; }
                    pos = end + 3;
                    continue;
                }

                // parse child element
                XmlNode grandchild;
                if (parseXml(xml, pos, grandchild))
                    ; // parseXml advances pos
                child.children.push_back(std::move(grandchild));
            }
        }

        node.children.push_back(std::move(child));
    }
    return true;
}

static XmlNode parseXmlString(const std::string& xml) {
    XmlNode root;
    size_t pos = 0;
    // skip XML declaration
    if (xml.find("<?xml") == 0) {
        pos = xml.find("?>") + 2;
    }
    parseXml(xml, pos, root);
    return root;
}

// --- ZIP reading (uses system `unzip` command) --------------------------------
// For a real implementation, use minizip, miniz, or libzip.

static bool extractFromZip(const std::string& zipPath,
                           const std::string& entryName,
                           std::vector<uint8_t>& outData)
{
    std::string cmd = "unzip -p \"" + zipPath + "\" \"" + entryName + "\" 2>/dev/null";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return false;

    outData.clear();
    uint8_t buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        outData.insert(outData.end(), buf, buf + n);

    int ret = pclose(fp);
    return ret == 0 && !outData.empty();
}

static bool listZipEntries(const std::string& zipPath,
                           std::vector<std::string>& entries)
{
    std::string cmd = "unzip -Z1 \"" + zipPath + "\" 2>/dev/null";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return false;

    entries.clear();
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        std::string s(line);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        if (!s.empty()) entries.push_back(s);
    }
    pclose(fp);
    return !entries.empty();
}

// --- Value mapping helpers ---------------------------------------------------

namespace BlissConvert {

    // Modulation destination: normalized float -> integer index
    // Formula: index = floor(value * 14)
    static const char* modDestNames[] = {
        "None", "Amp", "Pitch",
        "Filter1Freq", "Filter1Reso", "Filter1Drive",
        "Filter2Freq", "Filter2Reso", "Filter2Drive",
        "LFO1Rate", "LFO2Rate",
        "LoopStart", "LoopEnd", "Pan"
    };

    int modDestToIndex(float value) {
        int idx = static_cast<int>(value * 14.0f);
        return std::clamp(idx, 0, 13);
    }

    float modDestToFloat(int index) {
        return (static_cast<float>(index) + 0.5f) / 14.0f;
    }

    const char* modDestName(float value) {
        int idx = modDestToIndex(value);
        return modDestNames[idx];
    }

    // Filter cutoff: normalized 0-1 -> Hz
    // hz = 20 + x^2 * (22050 - 20)
    float filterCutoffToHz(float x) {
        return 20.0f + (x * x) * (22050.0f - 20.0f);
    }

    float filterCutoffFromHz(float hz) {
        return std::sqrt((hz - 20.0f) / (22050.0f - 20.0f));
    }

    // Envelope time: normalized 0-1 -> seconds
    // seconds = 0.001 + x^4 * 15.999
    float envTimeToSeconds(float x) {
        if (x <= 0.0f) return 0.001f;
        float x2 = x * x;
        return 0.001f + x2 * x2 * (16.0f - 0.001f);
    }

    float envTimeFromSeconds(float sec) {
        if (sec <= 0.001f) return 0.0f;
        return std::pow((sec - 0.001f) / (16.0f - 0.001f), 0.25f);
    }

    // Version hex: 0x030704 -> "3.7.4"
    std::string versionToString(int ver) {
        int major = (ver >> 16) & 0xFF;
        int minor = (ver >> 8) & 0xFF;
        int patch = ver & 0xFF;
        return std::to_string(major) + "." +
               std::to_string(minor) + "." +
               std::to_string(patch);
    }

    int versionFromString(const std::string& s) {
        // Parse "0x030704" hex format
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            return static_cast<int>(std::strtol(s.c_str(), nullptr, 16));
        return 0;
    }
}

// --- Data structures ---------------------------------------------------------

// RTPAR: value + sense stored as attributes on a child element
struct RtPar {
    float value = 0.0f;
    float sense = 0.5f;

    void parse(const XmlNode& node) {
        value = node.attrFloat("value", value);
        sense = node.attrFloat("sense", sense);
    }
};

struct BlissZone {
    std::string name;
    std::string path;

    int numChannels = 0;
    int numSamples  = 0;
    int sampleRate  = 44100;

    // Musical
    int mpGain = 0;
    int mpPan  = 0;

    // MIDI mapping
    int loKey = 0, hiKey = 127;
    int loVel = 0, hiVel = 127;

    // MIDI properties
    int midiTrigger    = 0;
    int midiRootKey    = 60;
    int midiFineTune   = 0;
    int midiCoarseTune = 0;
    int midiKeycents   = 100;

    // Loop
    int loopMode  = 0;
    int loopStart = 0;
    int loopEnd   = 0;
    float loopCrossfadeLen = 0.2f;

    // Resource groups
    int resGroup = 0;
    int resOffby = 0;

    // Filter 1
    int   flt1Type = 0;
    RtPar flt1CutFrq;
    RtPar flt1ResAmt;

    // Amplitude envelope
    RtPar ampEnvAtt, ampEnvDec, ampEnvSus, ampEnvRel, ampEnvAmt;
    RtPar ampEnvAttShp, ampEnvDecShp, ampEnvRelShp;

    // Amp env destinations (stored as normalized floats in zone attributes)
    float ampEnvDest1 = 0.0f;
    float ampEnvDest2 = 0.0f;
    RtPar ampEnvDest1Amt, ampEnvDest2Amt;

    // Velocity
    float velMod = 1.0f;
    float velAmp = 1.0f;

    // Unison
    int   unisonEnable = 0;
    int   unisonVoices = 4;
    float unisonDetune = 60.0f;

    void parse(const XmlNode& node) {
        // All scalar properties are XML attributes (JUCE ValueTree format)
        name = node.attr("name", "Untitled");
        path = node.attr("path", "");

        numChannels = node.attrInt("num_channels", 0);
        numSamples  = node.attrInt("num_samples", 0);
        sampleRate  = node.attrInt("sample_rate", 44100);

        mpGain = node.attrInt("mp_gain", 0);
        mpPan  = node.attrInt("mp_pan", 0);

        // MIDI input ranges are child elements with attributes
        if (auto* lo = node.child("lo_input_range")) {
            loKey = lo->attrInt("midi_key", 0);
            loVel = lo->attrInt("midi_vel", 0);
        }
        if (auto* hi = node.child("hi_input_range")) {
            hiKey = hi->attrInt("midi_key", 127);
            hiVel = hi->attrInt("midi_vel", 127);
        }

        // MIDI properties (attributes)
        midiTrigger    = node.attrInt("midi_trigger", 0);
        midiRootKey    = node.attrInt("midi_root_key", 60);
        midiFineTune   = node.attrInt("midi_fine_tune", 0);
        midiCoarseTune = node.attrInt("midi_coarse_tune", 0);
        midiKeycents   = node.attrInt("midi_keycents", 100);

        // Loop (attributes)
        loopMode  = node.attrInt("loop_mode", 0);
        loopStart = node.attrInt("loop_start", 0);
        loopEnd   = node.attrInt("loop_end", 0);
        loopCrossfadeLen = node.attrFloat("loop_crossfade_len", 0.2f);

        // Resource groups (attributes)
        resGroup = node.attrInt("res_group", 0);
        resOffby = node.attrInt("res_offby", 0);

        // Filter 1 type (attribute), cutoff/reso are RTPAR child elements
        flt1Type = node.attrInt("flt1_type", 0);
        if (auto* c = node.child("flt1_cut_frq")) flt1CutFrq.parse(*c);
        if (auto* c = node.child("flt1_res_amt")) flt1ResAmt.parse(*c);

        // Amp envelope: RTPAR child elements
        if (auto* c = node.child("amp_env_att"))     ampEnvAtt.parse(*c);
        if (auto* c = node.child("amp_env_att_shp")) ampEnvAttShp.parse(*c);
        if (auto* c = node.child("amp_env_dec"))     ampEnvDec.parse(*c);
        if (auto* c = node.child("amp_env_dec_shp")) ampEnvDecShp.parse(*c);
        if (auto* c = node.child("amp_env_sus"))     ampEnvSus.parse(*c);
        if (auto* c = node.child("amp_env_rel"))     ampEnvRel.parse(*c);
        if (auto* c = node.child("amp_env_rel_shp")) ampEnvRelShp.parse(*c);
        if (auto* c = node.child("amp_env_amt"))     ampEnvAmt.parse(*c);

        // Amp env destinations: stored as float attributes on zone
        ampEnvDest1 = node.attrFloat("amp_env_dest1", 0.03571f);
        ampEnvDest2 = node.attrFloat("amp_env_dest2", 0.03571f);
        // Destination amounts are RTPAR child elements
        if (auto* c = node.child("amp_env_dest1amt")) ampEnvDest1Amt.parse(*c);
        if (auto* c = node.child("amp_env_dest2amt")) ampEnvDest2Amt.parse(*c);

        // Velocity (attributes)
        velMod = node.attrFloat("vel_mod", 1.0f);
        velAmp = node.attrFloat("vel_amp", 1.0f);

        // Unison (attributes)
        unisonEnable = node.attrInt("unison_enable", 0);
        unisonVoices = node.attrInt("unison_voices", 4);
        unisonDetune = node.attrFloat("unison_detune", 60.0f);
    }
};

struct BlissProgram {
    std::string name;
    int         version  = 0;
    int         plyMode  = 2;
    int         numZones = 0;
    std::vector<BlissZone> zones;

    void parse(const XmlNode& programNode) {
        // Program properties are attributes (JUCE ValueTree)
        version  = BlissConvert::versionFromString(programNode.attr("version", "0"));
        name     = programNode.attr("name", "---");
        plyMode  = programNode.attrInt("ply_mode", 2);
        numZones = programNode.attrInt("num_zones", 0);

        // Parse zones: <zones> child contains <zone> children
        if (auto* zonesNode = programNode.child("zones")) {
            for (auto& zoneNode : zonesNode->children) {
                if (zoneNode.tag == "zone") {
                    BlissZone zone;
                    zone.parse(zoneNode);
                    zones.push_back(std::move(zone));
                }
            }
        }
    }
};

// --- Pretty-print helpers ----------------------------------------------------

static const char* loopModeName(int m) {
    static const char* names[] = {
        "OneShot", "Forward", "Bidirectional", "Backward",
        "Sustained", "Crossfade", "Static"
    };
    return (m >= 0 && m < 7) ? names[m] : "Unknown";
}

static const char* filterTypeName(int t) {
    static const char* names[] = {
        "Off", "Lowpass", "Hipass", "Bandpass1",
        "Bandpass2", "Notch", "Peak"
    };
    return (t >= 0 && t < 7) ? names[t] : "Unknown";
}

static const char* playModeName(int m) {
    static const char* names[] = { "Mono", "Legato", "Poly" };
    return (m >= 0 && m < 3) ? names[m] : "Unknown";
}

static std::string noteName(int midi) {
    static const char* notes[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    if (midi < 0 || midi > 127) return "?";
    int octave = (midi / 12) - 1;
    return std::string(notes[midi % 12]) + std::to_string(octave);
}

void printZone(const BlissZone& z, int index) {
    printf("  Zone %d: \"%s\"\n", index, z.name.c_str());
    printf("    Key range:  %s (%d) - %s (%d)   Root: %s (%d)\n",
           noteName(z.loKey).c_str(), z.loKey,
           noteName(z.hiKey).c_str(), z.hiKey,
           noteName(z.midiRootKey).c_str(), z.midiRootKey);
    printf("    Vel range:  %d - %d\n", z.loVel, z.hiVel);
    printf("    Tune:       coarse=%d st  fine=%d ct\n",
           z.midiCoarseTune, z.midiFineTune);
    printf("    Audio:      %d ch, %d frames, %d Hz\n",
           z.numChannels, z.numSamples, z.sampleRate);
    printf("    Loop:       %s  start=%d  end=%d  xfade=%.2f\n",
           loopModeName(z.loopMode), z.loopStart, z.loopEnd,
           z.loopCrossfadeLen);
    printf("    Gain:       %d dB   Pan: %d\n", z.mpGain, z.mpPan);

    // Filter
    if (z.flt1Type > 0) {
        printf("    Filter 1:   %s  cutoff=%.0f Hz  reso=%.1f%%\n",
               filterTypeName(z.flt1Type),
               BlissConvert::filterCutoffToHz(z.flt1CutFrq.value),
               z.flt1ResAmt.value * 100.0f);
    }

    // Envelope
    printf("    Amp Env:    A=%.3fs D=%.3fs S=%.0f%% R=%.3fs\n",
           BlissConvert::envTimeToSeconds(z.ampEnvAtt.value),
           BlissConvert::envTimeToSeconds(z.ampEnvDec.value),
           z.ampEnvSus.value * 100.0f,
           BlissConvert::envTimeToSeconds(z.ampEnvRel.value));
    printf("    Env shapes: A=%.2f D=%.2f R=%.2f  (0=concave 0.5=linear 1=convex)\n",
           z.ampEnvAttShp.value, z.ampEnvDecShp.value, z.ampEnvRelShp.value);

    // Amp env destinations
    int dest1 = BlissConvert::modDestToIndex(z.ampEnvDest1);
    int dest2 = BlissConvert::modDestToIndex(z.ampEnvDest2);
    if (dest1 > 0)
        printf("    Env Dest1:  %s  amount=%.1f%%\n",
               BlissConvert::modDestName(z.ampEnvDest1),
               (z.ampEnvDest1Amt.value - 0.5f) * 200.0f);
    if (dest2 > 0)
        printf("    Env Dest2:  %s  amount=%.1f%%\n",
               BlissConvert::modDestName(z.ampEnvDest2),
               (z.ampEnvDest2Amt.value - 0.5f) * 200.0f);

    // Velocity
    printf("    Velocity:   amp=%.2f  mod=%.2f\n", z.velAmp, z.velMod);

    // Unison
    if (z.unisonEnable > 0)
        printf("    Unison:     %d voices  detune=%.1f\n", z.unisonVoices, z.unisonDetune);

    // Resource groups
    if (z.resGroup > 0 || z.resOffby > 0)
        printf("    Group:      %d   Off_by: %d\n", z.resGroup, z.resOffby);

    // Source path
    if (!z.path.empty())
        printf("    Source:     %s\n", z.path.c_str());
}

void printProgram(const BlissProgram& prog, int index = -1) {
    if (index >= 0)
        printf("\n-- Program %d -------------------------------------------\n", index);
    else
        printf("\n-- Program ---------------------------------------------\n");

    printf("  Name:     \"%s\"\n", prog.name.c_str());
    printf("  Version:  %s (0x%06X)\n",
           BlissConvert::versionToString(prog.version).c_str(),
           prog.version);
    printf("  Play mode: %s\n", playModeName(prog.plyMode));
    printf("  Zones:    %d\n", static_cast<int>(prog.zones.size()));

    for (int z = 0; z < static_cast<int>(prog.zones.size()); z++)
        printZone(prog.zones[z], z);
}

// --- Main: detect file type and parse ----------------------------------------

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.zbp|file.zbb>\n", argv[0]);
        return 1;
    }

    std::string filePath = argv[1];
    std::string ext = std::filesystem::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != ".zbp" && ext != ".zbb") {
        fprintf(stderr, "Error: unsupported file extension '%s' (expected .zbp or .zbb)\n",
                ext.c_str());
        return 1;
    }

    printf("Bliss ZBP/ZBB Parser -- Reference Implementation\n");
    printf("File: %s\n", filePath.c_str());

    // -- ZBP: single program --------------------------------------------------
    if (ext == ".zbp") {
        std::vector<uint8_t> xmlData;
        if (!extractFromZip(filePath, "program.xml", xmlData)) {
            fprintf(stderr, "Error: could not read program.xml from ZIP\n");
            return 1;
        }

        std::string xmlStr(xmlData.begin(), xmlData.end());
        XmlNode root = parseXmlString(xmlStr);

        if (root.children.empty() || root.children[0].tag != "program") {
            fprintf(stderr, "Error: expected <program> root element\n");
            return 1;
        }

        BlissProgram prog;
        prog.parse(root.children[0]);
        printProgram(prog);

        // List embedded FLAC samples
        std::vector<std::string> entries;
        if (listZipEntries(filePath, entries)) {
            printf("\n-- Embedded samples ------------------------------------\n");
            for (auto& e : entries) {
                if (e.find(".flac") != std::string::npos)
                    printf("  %s\n", e.c_str());
            }
        }
    }

    // -- ZBB: bank with multiple programs -------------------------------------
    else if (ext == ".zbb") {
        // First read bank.xml for metadata
        std::vector<uint8_t> bankData;
        if (extractFromZip(filePath, "bank.xml", bankData)) {
            std::string bankXml(bankData.begin(), bankData.end());
            XmlNode bankRoot = parseXmlString(bankXml);

            if (!bankRoot.children.empty() && bankRoot.children[0].tag == "bank") {
                auto& bankNode = bankRoot.children[0];

                // Status is a child element with attributes
                if (auto* status = bankNode.child("status")) {
                    int interp = status->attrInt("interpolation", 0);
                    int curProg = status->attrInt("current_program", 0);
                    printf("Interpolation: %s\n",
                           interp == 0 ? "Normal" : interp == 1 ? "High" : "Extreme");
                    printf("Current program: %d\n", curProg);
                }

                // Programs are child elements within <programs>
                if (auto* progsNode = bankNode.child("programs")) {
                    int progIdx = 0;
                    for (auto& progNode : progsNode->children) {
                        if (progNode.tag == "program") {
                            BlissProgram prog;
                            prog.parse(progNode);

                            // Skip empty programs
                            if (prog.zones.empty() && prog.name == "---") {
                                progIdx++;
                                continue;
                            }

                            printProgram(prog, progIdx);
                            progIdx++;
                        }
                    }
                    printf("\nPrograms found: %d\n", progIdx);
                }
            }
        } else {
            // Fallback: try enumerating program_NNN/program.xml entries
            std::vector<std::string> entries;
            if (!listZipEntries(filePath, entries)) {
                fprintf(stderr, "Error: could not read bank.xml or list ZIP entries\n");
                return 1;
            }

            std::vector<std::string> programXmls;
            for (auto& e : entries) {
                if (e.find("program.xml") != std::string::npos &&
                    e.find("program_") != std::string::npos) {
                    programXmls.push_back(e);
                }
            }
            std::sort(programXmls.begin(), programXmls.end());

            printf("Programs found: %zu\n", programXmls.size());

            for (int p = 0; p < static_cast<int>(programXmls.size()); p++) {
                std::vector<uint8_t> xmlData;
                if (!extractFromZip(filePath, programXmls[p], xmlData))
                    continue;

                std::string xmlStr(xmlData.begin(), xmlData.end());
                XmlNode root = parseXmlString(xmlStr);

                if (root.children.empty()) continue;

                XmlNode* progNode = nullptr;
                for (auto& c : root.children) {
                    if (c.tag == "program") { progNode = &c; break; }
                }
                if (!progNode) continue;

                BlissProgram prog;
                prog.parse(*progNode);

                if (prog.zones.empty() && prog.name == "---")
                    continue;

                printProgram(prog, p);
            }
        }
    }

    printf("\nDone.\n");
    return 0;
}
