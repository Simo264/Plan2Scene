# Building and running the code

Installing dependencies for Wayland and X11.

Debian/Ubuntu:

```bash
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
```
Fedora:

```bash
sudo dnf install wayland-devel libxkbcommon-devel libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel
```
Installing CGAL devel library:

Debian/Ubuntu:

```bash
sudo apt-get install libcgal-dev
```

Fedora: 

```bash
sudo dnf install CGAL-devel
```


Compiling and running the code:

```bash
cmake . -B ./build 
cmake --build ./build/ --parallel 8
./build/Plan2Scene --load <model/input.gltf>
./build/Plan2Scene --parse <cad/input.dxf>
```

 It is also possible to convert a DWG file to DXF format using the `dwg2dxf` binary provided with the `libdxfrw` library:

```bash
./build/_deps/libdxfrw-build/dwg2dxf/dwg2dxf <input.dwg> <output.dxf>
```