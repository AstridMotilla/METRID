# METRID

**METRID** is a system for designing metamaterial actuators (robotic skins) using Conway operators. Its core is an algorithm that predicts which operator patterns produce auxetic (negative-Poisson-ratio) behavior.

Users can paint these patterns onto regions of a cylinder so that internal pressure produces controlled, region-specific deformation. Alternatively, they can specify a deformation goal and let METRID search for the pattern that best achieves it. Finished designs export to 3D-printable geometry for fabrication in a soft material.

METRID was built both as a development tool and to demonstrate the results of our paper:

> *Predicting Auxeticity from Conway Operator Words*

## Authors

- Astrid Motilla Monreal
- Daniela Abigail Lopez Mireles
- Giovanni Carlino

## Building the viewer

The current repository contains a simple OpenGL viewer (cylinder preview).

### Requirements

- CMake ≥ 3.10
- OpenGL
- GLFW3
- A C17 compiler

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

The binary will be placed in `build/bin/`.

```bash
./bin/METRID
```
