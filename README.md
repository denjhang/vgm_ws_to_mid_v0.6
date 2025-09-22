# vgm_ws_to_mid_v0.6
# vgm_ws_to_mid: WonderSwan VGM to MIDI Converter - Project Documentation

This document provides a detailed account of the development journey, technical implementation, and final program workflow of the `vgm_ws_to_mid` converter.

## 1. Development Journey: From Zero to a Flawless Converter

The goal of this project was to create a C++ program capable of accurately converting WonderSwan (WS) VGM files into MIDI files. The entire process was filled with challenges, but through a series of analysis, debugging, and iteration, we ultimately overcame them all.

### 1.1. Initial Exploration: Deconstructing the Core Logic

*   **Challenge**: Hardware documentation for the WonderSwan sound chip is scarce, making a direct conversion unfeasible.
*   **Solution**: In the early stages, we obtained the source code for the `modizer` project. By analyzing its file structure, we quickly located the key file: `modizer-master/libs/libwonderswan/libwonderswan/oswan/audio.cpp`. Through an in-depth study of this file, we successfully extracted the core information for simulating the WonderSwan sound chip:
    1.  **Clock Frequency**: Confirmed its master clock frequency is `3.072 MHz`.
    2.  **Register Functions**: Clarified the roles of key registers such as `0x80-0x87` (frequency), `0x88-0x8B` (volume), and `0x90` (channel switch).
    3.  **Waveform Table Mechanism**: Discovered the most critical detail—the frequency calculation must be divided by `32` (the size of the waveform table). This was the key to solving the pitch problem.

### 1.2. The First Major Hurdle: Correcting Pitch and Timing

*   **Challenge**: The initial version of the converter produced MIDI files with abnormally high pitch and a playback duration that did not match the original VGM at all.
*   **Breakthrough Process**:
    1.  **Timing Issue**: We realized that the VGM "wait" command (`0x61 nn nn`) is based on samples at 44100 Hz, while MIDI time is measured in `ticks`. By introducing a conversion factor `SAMPLES_TO_TICKS = (480.0 * 120.0) / (44100.0 * 60.0)`, we successfully converted the sample count into precise MIDI ticks under the standard 120 BPM and 480 PPQN, resolving the duration mismatch.
    2.  **Pitch Issue**: This was the toughest problem. The initial frequency conversion formula `freq = 3072000.0 / (2048.0 - period)` resulted in a pitch that was a full five octaves too high. After repeatedly reviewing the `modizer`'s `audio.cpp` source, we noticed a detail: the final frequency value was used as an index for a waveform table. This led us to the insight that the actual audible frequency is the result of the clock frequency after division and waveform table processing. By adding `/ 32` to the end of our formula, we obtained the correct frequency, and the pitch problem was solved.

### 1.3. The Silent MIDI: Decrypting Dynamic Volume

*   **Challenge**: After implementing volume control, the generated MIDI files became silent in many players, or the volume changes did not behave as expected.
*   **Breakthrough Process**:
    1.  **Problem-Solving**: Initially, we used MIDI CC#7 (Main Volume) to handle volume changes during a note's duration. However, many MIDI synthesizers treat CC#7 as a static setting for a channel rather than a real-time "expression" parameter, leading to compatibility issues.
    2.  **Solution**: By consulting MIDI specifications and best practices, we confirmed that CC#11 (Expression) is the standard controller for handling dynamic note envelopes. After changing the code from `add_control_change(channel, 7, ...)` to `add_control_change(channel, 11, ...)`, the MIDI files correctly exhibited dynamic volume changes on all players, and the silence issue was completely resolved.

### 1.4. The Final Polish: The Art of Volume Mapping

*   **Challenge**: Even with dynamic volume implemented, the vast difference in dynamic range between WonderSwan's 4-bit volume (0-15) and MIDI's 7-bit volume (0-127) caused a direct linear mapping to result in an overall low volume, making the music sound "weak".
*   **Breakthrough Process**:
    1.  **Problem Analysis**: A linear mapping `midi_vol = vgm_vol / 15.0 * 127.0` mapped a large portion of the mid-to-low range VGM volumes to very low, barely audible MIDI values.
    2.  **Non-linear Mapping**: To boost overall audibility while preserving dynamic range, we introduced a power function `pow(normalized_vol, exponent)` as a non-linear mapping curve. Through experimentation, we found:
        *   `exponent = 0.6`: A good starting point that effectively boosted low volumes, but was still not loud enough per user feedback.
        *   `exponent = 0.3`: A more aggressive curve that dramatically enhanced the expressiveness of mid-to-low volumes while still maintaining headroom at maximum volume to avoid clipping.
    3.  **Iteration via User Feedback**: Based on the final user feedback ("raise the pitch by one octave and keep increasing the volume"), we removed the experimental `-12` pitch offset and adopted the `exponent = 0.3` volume curve, finally achieving a perfect result that satisfied the user.

### 1.5. The Self-Correcting Feedback Loop: The Power of Custom Validation Tools

You astutely pointed out the key to this project's success: we didn't just write a converter; more importantly, we created tools for validation and debugging. This established a powerful and rapid "Code-Test-Validate" feedback loop, enabling us to objectively and efficiently discover and solve problems, rather than relying on subjective listening.

**Core Debugging Tool: `midi_validator.exe`**

This is a lightweight MIDI parser written from scratch. Its functionality evolved as the project progressed:

*   **Initial Function**: Checked the basic structural integrity of the MIDI file, ensuring the header (MThd) and track chunks (MTrk) were not corrupted.
*   **Core Function**: The biggest breakthrough was adding a detailed **event logging** feature. It could list every single MIDI event (Note On, Note Off, Control Change, etc.) in a clear, chronological, line-by-line format, displaying its precise tick time, channel, and data values.

**How Did It Help Us?**

1.  **Validating Timing**: By examining the `Tick` column in the log, we could precisely verify the correctness of the `SAMPLES_TO_TICKS` formula.
2.  **Validating Pitch**: The `Data 1` column for `Note On` events directly showed the MIDI pitch number, allowing us to objectively judge the accuracy of the pitch conversion instead of just "how it sounds."
3.  **Debugging Volume**: This was its most critical use. By observing the velocity (`Data 2`) of `Note On` events and the values of CC#11 events, we could quantify the volume level. This allowed us to pinpoint the root cause of the "silent MIDI" issue (CC#7 vs. CC#11) and scientifically tune the exponent of our non-linear volume curve until the output values fell within the desired range.

**The Feedback-Driven Debugging Workflow**

This tool made our debugging process efficient and scientific:

1.  **Modify**: Adjust the conversion logic in `WonderSwanChip.cpp`.
2.  **Compile**: Recompile `converter.exe`.
3.  **Generate**: Run the converter to produce a new `output.mid`.
4.  **Validate**: **Immediately run `midi_validator.exe output.mid`** to get an objective "health report" on the new file.
5.  **Analyze**: Compare the log against expectations to confirm if the changes were effective and if any new issues were introduced.
6.  **Iterate**: Based on the analysis, proceed with the next round of modifications.

This data-driven iterative approach was the core methodology that enabled us to overcome numerous tricky technical hurdles and ultimately achieve a near-perfect result.

### 1.6. An Example Trace: The Life of a VGM Command Sequence

To understand the conversion process more concretely, let's trace a small, typical sequence of VGM commands and see how they become MIDI events.

Assume our converter receives the following VGM command sequence at `tick = 100`:

| VGM Command (Hex) | Meaning | Internal State Change & Action |
| :--- | :--- | :--- |
| `51 88 88` | Write `0x88` to port `0x88` | `WonderSwanChip` updates: `channel_volumes_left[0] = 8`, `channel_volumes_right[0] = 8`. |
| `51 80 E0` | Write `0xE0` to port `0x80` | `WonderSwanChip` updates: the low 8 bits of `channel_periods[0]` become `0xE0`. |
| `51 81 06` | Write `0x06` to port `0x81` | `WonderSwanChip` updates: the high 3 bits of `channel_periods[0]` become `0x06`. The period value is now `0x6E0` (1760). |
| `51 90 01` | Write `0x01` to port `0x90` | `WonderSwanChip` updates: `channel_enabled[0] = true`. **The state machine detects an "on" signal** with volume > 0. |
| `61 F4 01` | Wait for 500 samples (~11.3 ms) | `WonderSwanChip` accumulates time. The note continues to sound. |
| `51 88 44` | Write `0x44` to port `0x88` | `WonderSwanChip` updates: `channel_volumes_left[0] = 4`, `channel_volumes_right[0] = 4`. **The state machine detects a volume change**. |
| `61 F4 01` | Wait for 500 samples (~11.3 ms) | `WonderSwanChip` accumulates time. The note continues to sound at the new volume. |
| `51 90 00` | Write `0x00` to port `0x90` | `WonderSwanChip` updates: `channel_enabled[0] = false`. **The state machine detects an "off" signal**. |

**Generated MIDI Event Sequence:**

| Tick | MIDI Event | Channel | Data 1 (Pitch/CC#) | Data 2 (Velocity/Value) | Remarks |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 100 | `Note On` | 0 | 65 (F4) | 105 | Note begins. Pitch is calculated from period `1760`, velocity is derived from volume `8` via non-linear mapping. |
| 124 | `Control Change` | 0 | 11 (Expression) | 88 | During the note, volume drops from `8` to `4`, triggering an expression controller event. |
| 148 | `Note Off` | 0 | 65 (F4) | 0 | The channel is disabled, triggering a note-off event. |

This example clearly demonstrates how the converter operates like a true chip emulator, intelligently generating a MIDI stream in real-time by responding to register state changes.

## 2. Program Workflow Explained

The core of `vgm_ws_to_mid` is a state machine that simulates the behavior of the WonderSwan sound chip and translates its state changes into MIDI events in real-time.

### 2.1. Overview

1.  **Read**: `VgmReader` reads the VGM file byte by byte, parsing out commands (e.g., port writes, waits).
2.  **Process**: The `main` function loops through the VGM commands.
    *   If it's a **wait command**, it accumulates time.
    *   If it's a **port write command**, it sends it to the `WonderSwanChip`.
3.  **Simulate & Translate**: `WonderSwanChip` receives the port write data and updates its internal register states (e.g., frequency, volume). After each update, it checks if the channel's state has changed (e.g., note on/off, pitch change, volume change).
4.  **Generate**: If a meaningful state change is detected, `WonderSwanChip` calls `MidiWriter` to generate the corresponding MIDI event (Note On/Off, Control Change) with the current, precise MIDI tick time.
5.  **Write**: After all VGM commands are processed, `MidiWriter` assembles all generated events into a standard MIDI file and saves it to disk.

### 2.2. Key Components

*   **`main.cpp`**: The program entry point. It's responsible for parsing command-line arguments, instantiating `VgmReader`, `MidiWriter`, and `WonderSwanChip`, and driving the entire conversion process.
*   **`VgmReader.h/.cpp`**: The VGM file parser. It reads the file as a stream, handling data blocks and various VGM commands, abstracting away the complexity of the file format.
*   **`WonderSwanChip.h/.cpp`**: The **conversion core**.
    *   It maintains an `io_ram` array to simulate the chip's 256 I/O registers.
    *   The `write_port()` method is the key entry point, updating internal state variables (like `channel_periods`, `channel_volumes_left`, etc.) based on the port address being written to.
    *   `check_state_and_update_midi()` is the brain of the state machine. After each state update, it compares the current state to the previous one to determine if a MIDI event needs to be generated, thus intelligently handling legato, re-triggers, and volume envelopes.
*   **`MidiWriter.h/.cpp`**: The MIDI file generator. It provides a simple set of APIs (like `add_note_on`, `add_control_change`) to build a MIDI track in memory. When the conversion is finished, the `finalize_and_write()` method calculates track lengths, adds headers and footers, and writes a correctly formatted SMF (Standard MIDI File).

### 2.3. Key Formulas and Constants

*   **Timing Conversion**:
    `const double SAMPLES_TO_TICKS = (480.0 * 120.0) / (44100.0 * 60.0);`
*   **Pitch Conversion**:
    `double freq = (3072000.0 / (2048.0 - period)) / 32.0;`
    `int note = static_cast<int>(round(69 + 12 * log2(freq / 440.0)));`
*   **Volume Mapping**:
    `double normalized_vol = vgm_vol / 15.0;`
    `double curved_vol = pow(normalized_vol, 0.3);`
    `int velocity = static_cast<int>(curved_vol * 127.0);`

## 3. How to Compile and Run

This project is compiled using g++ in a bash environment.

*   **Compile**:
    ```bash
    g++ -std=c++17 -o vgm_ws_to_mid/converter.exe vgm_ws_to_mid/main.cpp vgm_ws_to_mid/VgmReader.cpp vgm_ws_to_mid/WonderSwanChip.cpp vgm_ws_to_mid/MidiWriter.cpp -static
    ```
*   **Run**:
    ```bash
    vgm_ws_to_mid/converter.exe [input_vgm_file] [output_mid_file]
    ```
    For example:
    ```bash
    vgm_ws_to_mid/converter.exe inn.vgm vgm_ws_to_mid/output.mid
    ```

---
This document provides a comprehensive summary of our work. We hope it serves as a clear guide for future development and maintenance.
