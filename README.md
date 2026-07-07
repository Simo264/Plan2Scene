## Screenshots

![screen 1](screenshots/img_1.png)

![screen 2](screenshots/img_2.png)

![screen 3](screenshots/img_3.png)

![screen 4](screenshots/img_4.png)

![screen 5](screenshots/img_5.png)

![screen 6](screenshots/img_6.png)

![screen 7](screenshots/img_7.png)

![screen 8](screenshots/img_8.png)

## Configuration

Use a :

All program parameters are now readable from a template-specific configuration file.
Example of 'config.json':

```json
{
  "dxf_file": "file.dxf",
  "unit_scale": 0.1,
  "snap_eps": 1e-4,
  "cluster_num_samples": 10,
  "cluster_eps": 1.0,
  "ceil_height": 10.0,
  "floor_texture_scaling": 2.0,
  "wall_texture_scaling": 2.0
}
```

## Building

Install Conan package manager with pip:

```bash
pip install conan
```

This will detect the operating system, build architecture and compiler settings based on the environment.
It will also set the build configuration as Release by default

```bash
conan profile detect --force
```

We will use Conan to install dependencies and generate the files that CMake needs to find these libraries and build our project.
We will generate those files in the folder **build**.

```bash
conan install . \
  --output-folder=build \
  --build=missing \
  -s build_type=Debug \
  -c tools.system.package_manager:mode=disabled
```

Conan generated several files under the **build** folder.

To build the project:

```bash
cmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=build/build/Debug/generators/conan_toolchain.cmake
```

To compile:

```bash
cmake --build ./build/ --parallel 8
```

## Run

The program accepts a JSON configuration file as an argument:

```bash
./build/Plan2Scene cad/house_plan/Simple_House_Plan.json
```

```bash
blender -b -P render_scene.py
```