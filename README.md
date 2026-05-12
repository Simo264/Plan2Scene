## Building and running the code

Installing dependencies for Wayland and X11.

On Debian/Ubuntu:

```bash
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
```
On Fedora:

```bash
sudo dnf install wayland-devel libxkbcommon-devel libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel
```

Compiling and running the code:

```bash
cmake . -B ./build 
cmake --build ./build/ --parallel 8
./build/Plan2Scene --load <model/input.gltf>
./build/Plan2Scene --parse <cad/input.dxf>
```