# Minimalistic Animation Software

A lightweight desktop animation and drawing app built with **C++23** and **Qt 6**.  
The goal of this project is to create a simple, responsive animation tool with a clean interface, smooth pen input, frame-by-frame drawing, and essential animation features.

This app is currently in early development, with the main focus on building a strong canvas and frame system before adding more advanced animation tools.

## Project Goal

The goal is to build a minimal animation software app inspired by tools like Krita and OneNote, while keeping the interface simple and focused.

The core idea is:

> Open the app, draw on a canvas, create new frames, animate frame-by-frame, preview the animation, and export it.

The brush feel is an important part of the project. The long-term goal is to make drawing feel smooth, natural, and responsive, similar to the OneNote pen tool.

## Current Features

- Basic Qt desktop application
- Main window layout with canvas area and controls
- Drawing canvas
- Frame-based drawing system
- Each animation frame stores its own unique drawing data
- Ability to add and switch between frames
- Basic timeline-style frame navigation

## Planned Features

- Smooth brush engine
- Pressure-sensitive pen input
- Eraser tool
- Colour selection
- Undo and redo
- Onion skinning
- FPS playback controls
- Move/adjust tool
- Save and load projects
- PNG sequence export
- MP4/GIF/WebM export using FFmpeg
- Future Skia rendering backend for better performance

## Tech Stack

- **C++23** - Core application logic
- **Qt 6** - Desktop UI, widgets, input handling, and canvas rendering
- **CMake** - Build system
- **Windows Ink / Qt Tablet Events** - Planned stylus and pressure input support
- **Skia** - Planned high-performance 2D rendering backend
- **SQLite / Custom Project Format** - Planned project saving system
- **FFmpeg** - Planned animation/video export

## Roadmap

### Phase 1: App Shell

Create the main desktop window, toolbar, canvas area, and timeline section.

### Phase 2: Canvas

Implement basic drawing on the canvas using Qt painting.

### Phase 3: Brush Engine

Improve the brush feel with smoothing, round caps, pressure support, and stabilization.

### Phase 4: Basic Tools

Add colour selection, eraser, undo, and redo.

### Phase 5: Frames and Timeline

Add support for multiple animation frames where each frame has its own unique drawing data.

### Phase 6: Onion Skin and Playback

Allow previous and next frames to appear faintly behind the current frame, and add FPS-based playback.

### Phase 7: Save and Load

Save animation projects and reload them later.

### Phase 8: Export

Export animations as PNG sequences first, then add FFmpeg support for MP4, GIF, and WebM.

### Phase 9: Rendering Polish

Replace basic Qt painting with a Skia renderer for better performance and future features.

## MVP Target

The minimum usable version of this app should allow users to:

- Draw on a canvas
- Create multiple animation frames
- Keep drawings unique to each frame
- Switch between frames
- Use onion skinning
- Play the animation at a chosen FPS
- Save and load projects
- Export the animation

## Build Requirements

To build the project, you need:

- Visual Studio 2022
- CMake
- Qt 6
- A C++23-compatible compiler

## Build Instructions

Clone the repository:

```bash
git clone https://github.com/chubomis/minimal-animation-software.git
cd minimal-animation-software
