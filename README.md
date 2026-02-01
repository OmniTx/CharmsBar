# Charms Bar Platinum (AHK)

A faithful and enhanced recreation of the Windows 8 Charms Bar for Windows 10 and 11, built with AutoHotkey v2. 

## Features & Options

### 1. **Search Integration**
- **Windows Search**: Default. Opens the native Windows Start Menu search.
- **Everything Search**: **(Crucial Feature)** Seamlessly integrates with [Everything](https://www.voidtools.com/) by voidtools. If configured, clicking the Search icon launches Everything for instant file finding.
    - *Why use this?* Windows Search can be slow. "Everything" is instant.

### 2. **Startup & "No UAC" Mode**
- **Launch on Boot**: Can be configured to start automatically with Windows.
- **No UAC Prompt**: The script uses Windows Task Scheduler (`schtasks`) to bypass the annoying User Account Control (UAC) prompt on startup, ensuring silent and seamless activation.

### 3. **Custom Apps (A, B, C)**
- The bar includes 3 customizable quick-launch slots at the bottom.
- You can assign **Name** (displayed on hover) and **Path** (exe to run) for each button via the Settings menu.

### 4. **Theme & Appearance**
- **Modes**:
    - **Dark/Light**: Standard clean look.
    - **Image**: Choose any image file to set as the background.
    - **Gradient**: Define a start/end hex color for a static gradient.
    - **Custom**: Set a specific solid hex color.
- **Text Color**: Auto-detected or force White/Black for readability on custom backgrounds.

### 5. **Tools & Media**
- **Media**: Dedicated Volume Slider, Play/Pause, Next, Prev.
- **Tools**: Quick access to **Snipping Tool**, **Calculator**, and **Task Manager**.

### 6. **Multi-Monitor & Touch Support**
- **Multi-Monitor**: The bar detects which monitor your mouse is on and opens on that specific screen, ensuring you can access it anywhere.
- **Touch Friendly**: The interface features large, tap-friendly buttons and sliders, making it ideal for Windows tablets and touchscreens.

## Configuration

1. **Open Settings**: Open the Charms Bar (Win+C) -> Click the **Gear Icon** (bottom right).
2. **Settings Menu**:
    - **Run at Startup**: Toggles the Task Scheduler entry.
    - **Search Provider**: Select "Windows" or "Everything". If "Everything", provide the path to `Everything.exe`.
    - **Custom Apps**: Click "..." to browse for your favorite apps to plug into slots A, B, and C.
    - **Appearance**: Select your preferred Background Mode and Colors.
3. **Save**: Click "Save & Apply" to write changes to `CharmsSettings.ini` and reload immediately.

## Usage
- **Mouse**: Hover the cursor in the **top-right corner** of the screen for 0.4 seconds.
- **Keyboard**: Press `Win+C` to toggle.
- **Center Window**: Press `Win+Alt+C` to instantly center the active window on the screen.
- **Touch**: Tap the corner or use `Win+C`. The UI is optimized for touch usage.

## File Information
- `CharmsSettings.ini`: configurations are saved here.
- `No UAC`: Uses admin privileges only when setting up the task; runs silently afterwards.

---
**Credits**:
- **Author**: Imran Ahmed
    - GitHub: [omnitx](https://github.com/omnitx)
    - Email: [imranomnitx@duck.com](mailto:imranomnitx@duck.com)
- **Co-Developer**: Antigravity (Google DeepMind)
