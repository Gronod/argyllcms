# ArgyllCMS `chartread -u` Subprocess Integration Specification & License Isolation Guide

This document specifies the communication protocol, JSON payload schema, process lifecycle, and licensing isolation architecture for integrating `chartread -u` into external user interfaces and control software.

---

## 1. Architectural & Licensing Boundary (AGPLv3 Isolation)

ArgyllCMS is licensed under the **GNU Affero General Public License (AGPL) Version 3**. To ensure a separate host project (proprietary, MIT, Apache, etc.) is **not tainted** by the AGPLv3 copyleft terms, the integration must maintain a strict, arm's-length inter-process communication (IPC) boundary:

### Core Isolation Rules
1. **No Library Linking**: The host application **must never** statically or dynamically link (`#include`, `.so`, `.dylib`, `.dll`, `.a`) against any ArgyllCMS C libraries (`libinst`, `libicc`, `libcgats`, `libyajl`, etc.).
2. **Subprocess Isolation via Standard OS Pipes**: `chartread` must run strictly as a standalone, decoupled child process.
3. **Standard IPC Only**: Communication is conducted exclusively over standard POSIX / Win32 file descriptors (`stdin`, `stdout`, `stderr`).
4. **Independent Binary Distribution**: The `chartread` binary should be treated as an external utility tool invoked by the OS shell or process manager.

```
+-------------------------------------------------------------+
|                     Host Application                        |
|       (Electron, Web App, Qt, Python, Rust, etc.)          |
|                                                             |
|   +-----------------------------------------------------+   |
|   |         Line-by-Line Subprocess Stream Reader       |   |
|   +-----------------------------------------------------+   |
+------------------------------|------------------------------+
                               |
               Standard Pipes  |  (stdin / stdout / stderr)
                               |
+------------------------------v------------------------------+
|                   Isolated Child Process                    |
|                      `chartread -u ...`                     |
|                   (AGPLv3 Licensed Binary)                  |
+-------------------------------------------------------------+
```

---

## 2. Command Invocation

### Command Syntax
```bash
chartread [options] -u <target_basename>
```

* **`-u` Flag**: Enables real-time emission of JSON records to `stdout`.
* `<target_basename>`: The base name of the input `.ti2` target file and output `.ti3` measurement file (without extension).

### Key Command Options
| Flag | Description |
| :--- | :--- |
| `-u` | **Required for streaming**: Emits `ROW_COLORS_JSON: ...` payloads on `stdout`. |
| `-p` | Patch-by-patch spot mode (emits JSON after each individual patch). |
| `-n` | Disable spectral readings (omits `spectral` field from JSON). |
| `-c <port>` | Select communication port / instrument index. |
| `-d` | Display measurement mode. |
| `-t` | Transmission measurement mode. |
| `-e` | Emissive measurement mode. |

---

## 3. Stream Protocol & Framing Specification

When `-u` is provided, `chartread` outputs two types of lines to `stdout`:

1. **Human-Readable Status / Prompt Lines**: Unprefixed text intended for user prompts or diagnostics (e.g. `Hit [Space] to read strip A`).
2. **Structured JSON Events**: Single-line JSON objects strictly prefixed by the marker:
   ```
   ROW_COLORS_JSON: <json_object>\n
   ```

### Stream Guarantees
* **Line-Delimited**: Every JSON event is serialized as a single, uninterrupted line terminated with `\n`.
* **Immediate Flush**: `chartread` explicitly invokes `fflush(stdout)` immediately after emitting each JSON line, guaranteeing low latency without OS-level output buffering.
* **Deterministic Ordering**: Patches within a row are always indexed in canonical left-to-right strip order (bi-directional scan reversals are normalized internally before emission).

---

## 4. JSON Payload Schema

### Top-Level Object Schema

```json
ROW_COLORS_JSON: {
  "event": "row_complete",
  "row_id": "A",
  "row_index": 0,
  "total_rows": 12,
  "patch_count": 21,
  "patches": [
    /* Array of Patch Objects */
  ]
}
```

| Field | Type | Description |
| :--- | :--- | :--- |
| `event` | `string` | Event identifier. Currently always `"row_complete"`. |
| `row_id` | `string` | Human-readable label of the row/strip (e.g. `"A"`, `"B"`, `"1"`). |
| `row_index` | `integer` | 0-based index of the row within the target chart (`0 ... total_rows - 1`). |
| `total_rows` | `integer` | Total number of rows/passes defined in the target chart. |
| `patch_count`| `integer` | Number of patch elements contained in this row payload. |
| `patches` | `array` | List of individual patch data objects. |

---

### Patch Object Schema

```json
{
  "id": "1",
  "loc": "A1",
  "is_pad": false,
  "device": [0.0, 50.0, 100.0, 0.0],
  "expected": {
    "XYZ": [18.4210, 20.1234, 15.6789],
    "Lab": [51.98, -8.45, 12.32]
  },
  "measured": {
    "XYZ": [18.5120, 20.0451, 15.7100],
    "Lab": [51.89, -8.31, 12.15],
    "spectral": {
      "bands": 36,
      "start_nm": 380.0,
      "end_nm": 730.0,
      "norm": 100.0,
      "values": [0.0120, 0.0135, 0.0180, 0.0245]
    }
  }
}
```

| Field | Type | Description |
| :--- | :--- | :--- |
| `id` | `string` | Patch ID string from `.ti2` file. If `"0"`, indicates a spacer/padding patch. |
| `loc` | `string` | Physical location coordinate string (e.g. `"A1"`, `"B12"`). |
| `is_pad` | `boolean` | `true` if patch is an alignment/lead-in spacer patch (`id == "0"`). UIs should typically render these distinctly or skip them in analysis. |
| `device` | `array[float]` | Device colorant drive values scaled to percentage `0.0 ... 100.0%` (e.g. `[C, M, Y, K]` or `[R, G, B]`). |
| `expected` | `object` *(optional)* | Expected reference values from `.ti2` (omitted if no reference data is present). |
| `expected.XYZ` | `array[float][3]` | Reference CIE XYZ values on reference scale `0.0 ... 100.0`. |
| `expected.Lab` | `array[float][3]` | Reference D50 $L^*a^*b^*$ computed via standard CIE transformation. |
| `measured` | `object` | Actual instrument readings. |
| `measured.XYZ` | `array[float][3]` | Measured CIE XYZ values on reference scale `0.0 ... 100.0`. |
| `measured.Lab` | `array[float][3]` | Measured D50 $L^*a^*b^*$ ($L^* \in [0, 100]$, $a^*, b^* \in [-128, 127]$). |
| `measured.spectral` | `object` *(optional)* | Spectral reflection/emission data (omitted if instrument is colorimeter-only or `-n` passed). |
| `measured.spectral.bands` | `integer` | Number of spectral sample bands. |
| `measured.spectral.start_nm` | `float` | Starting wavelength in nanometres (e.g. `380.0` or `400.0`). |
| `measured.spectral.end_nm` | `float` | Ending wavelength in nanometres (e.g. `700.0` or `730.0`). |
| `measured.spectral.norm` | `float` | Normalization scale factor (typically `100.0`). |
| `measured.spectral.values` | `array[float]` | Array of spectral reflectance / radiance values per band. |

---

## 5. Integration Implementation Examples

### Node.js / Electron / TypeScript Integration

```typescript
import { spawn, ChildProcessWithoutNullStreams } from 'child_process';
import * as readline from 'readline';

export interface PatchColorEvent {
  event: string;
  row_id: string;
  row_index: number;
  total_rows: number;
  patch_count: number;
  patches: Array<{
    id: string;
    loc: string;
    is_pad: boolean;
    device: number[];
    expected?: {
      XYZ: [number, number, number];
      Lab: [number, number, number];
    };
    measured: {
      XYZ: [number, number, number];
      Lab: [number, number, number];
      spectral?: {
        bands: number;
        start_nm: number;
        end_nm: number;
        norm: number;
        values: number[];
      };
    };
  }>;
}

export class ChartreadRunner {
  private process: ChildProcessWithoutNullStreams | null = null;
  private readonly JSON_PREFIX = 'ROW_COLORS_JSON: ';

  public startSession(
    targetBasename: string,
    onRowData: (data: PatchColorEvent) => void,
    onConsoleMessage: (msg: string) => void,
    onError: (err: string) => void
  ): void {
    // Spawn isolated process over standard pipes (No library linking)
    this.process = spawn('chartread', ['-u', targetBasename]);

    // Parse stdout line by line
    const rlOut = readline.createInterface({ input: this.process.stdout });
    rlOut.on('line', (line: string) => {
      const trimmed = line.trim();
      if (trimmed.startsWith(this.JSON_PREFIX)) {
        try {
          const jsonStr = trimmed.substring(this.JSON_PREFIX.length);
          const payload: PatchColorEvent = JSON.parse(jsonStr);
          onRowData(payload);
        } catch (err) {
          onError(`Failed to parse JSON row payload: ${err}`);
        }
      } else if (trimmed.length > 0) {
        onConsoleMessage(trimmed);
      }
    });

    // Capture stderr for warnings / errors
    const rlErr = readline.createInterface({ input: this.process.stderr });
    rlErr.on('line', (errLine: string) => {
      onError(errLine);
    });

    this.process.on('close', (code) => {
      onConsoleMessage(`chartread exited with code ${code}`);
      this.process = null;
    });
  }

  /**
   * Send user keyboard triggers or confirmations to chartread (e.g. Spacebar or Enter)
   */
  public sendInput(input: string): void {
    if (this.process && this.process.stdin.writable) {
      this.process.stdin.write(input);
    }
  }

  /**
   * Abort measurement session
   */
  public abort(): void {
    if (this.process) {
      this.sendInput('q\n'); // Send standard quit character
      setTimeout(() => {
        if (this.process) {
          this.process.kill('SIGTERM');
        }
      }, 500);
    }
  }
}
```

---

### Python Subprocess Integration

```python
import subprocess
import json
import threading

class ChartreadClient:
    JSON_PREFIX = "ROW_COLORS_JSON: "

    def __init__(self, target_basename: str, on_row_callback, on_status_callback):
        self.target_basename = target_basename
        self.on_row_callback = on_row_callback
        self.on_status_callback = on_status_callback
        self.process = None

    def start(self):
        # Arms-length subprocess invocation maintaining AGPL isolation
        self.process = subprocess.Popen(
            ["chartread", "-u", self.target_basename],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1  # Line buffered
        )

        threading.Thread(target=self._read_stdout, daemon=True).start()
        threading.Thread(target=self._read_stderr, daemon=True).start()

    def _read_stdout(self):
        for line in iter(self.process.stdout.readline, ''):
            line_str = line.strip()
            if line_str.startswith(self.JSON_PREFIX):
                payload_str = line_str[len(self.JSON_PREFIX):]
                try:
                    data = json.loads(payload_str)
                    self.on_row_callback(data)
                except json.JSONDecodeError as ex:
                    print(f"JSON decode error: {ex}")
            elif line_str:
                self.on_status_callback(line_str)

    def _read_stderr(self):
        for line in iter(self.process.stderr.readline, ''):
            if line.strip():
                print(f"[chartread stderr] {line.strip()}")

    def send_key(self, key: str):
        if self.process and self.process.stdin:
            self.process.stdin.write(f"{key}\n")
            self.process.stdin.flush()

    def terminate(self):
        if self.process:
            self.process.terminate()
```

---

## 6. Real-Time UI Visualisation Best Practices

1. **Rendering Device Colors**:
   * For RGB targets: scale `device[0..2]` from $0..100$ to $0..255$ (`rgb(r%, g%, b%)`).
   * For CMYK targets: use device simulation or convert `measured.Lab` / `expected.Lab` to sRGB for color swatch display.
2. **Delta E Calculation ($\Delta E_{00}$ or $\Delta E_{ab}$)**:
   * When `expected.Lab` and `measured.Lab` are both present, calculate the color difference $\Delta E$.
   * The basic Euclidean distance ($\Delta E_{ab}$) is simple to implement directly:
     $$\Delta E_{ab} = \sqrt{(L_m^* - L_e^*)^2 + (a_m^* - a_e^*)^2 + (b_m^* - b_e^*)^2}$$
   * **Recommendation**: For professional color work, use the modern $\Delta E_{00}$ (CIEDE2000) formula. Due to its complexity, it is highly recommended to use an established color math library (e.g., `colorjs.io` in JavaScript, or `colormath` in Python) rather than implementing it from scratch.
   * Display green/amber/red indicator lights next to each patch in real-time based on your accepted $\Delta E$ tolerance.
3. **Handling Alignment / Spacer Patches (`is_pad == true`)**:
   * Omit `is_pad == true` patches from quality score computations, or render them with dashed neutral borders to maintain layout grid accuracy without skewing statistics.
