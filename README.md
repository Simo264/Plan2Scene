## Screenshots

![primitives_extraction](screenshots/primitives_extraction.png)

![clustering](screenshots/clustering.png)

![faces_extraction](screenshots/faces_extraction.png)

![preview_textured_mesh](screenshots/preview_textured_mesh.png)

![preview_mesh_with_lighting](screenshots/preview_mesh_with_lighting.png)

![draftperson_Floor_Plan_cycles](screenshots/draftperson_Floor_Plan_cycles.png)

![Simple_House_Plan_cycles](screenshots/Simple_House_Plan_cycles.png)

![Door_Placeholders](screenshots/door_placeholders.png)

![Window_Placeholders](screenshots/window_placeholders.png)

![Door_Assets](screenshots/door_assets.png)

![Door_Window_Assets](screenshots/door_window_assets.png)

![draftperson_with_assets](screenshots/draftperson_with_assets.png)

## Building

Install Conan package manager with pip:

```bash
pip install conan
```

Detect your system profile:

```bash
conan profile detect --force
```

Install dependencies and generate build files:

```bash
conan install . \
  --output-folder=build \
  --build=missing \
  -s build_type=Debug \
  -c tools.system.package_manager:mode=disabled
```

Configure CMake using the Conan toolchain:

```bash
cmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=build/build/Debug/generators/conan_toolchain.cmake
```

Compile the project:

```bash
cmake --build ./build/ --parallel 8
```

## Run

Before executing the program, create and activate a Python virtual environment (used for visualisation scripts):

```bash
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```
Launch the program by passing the JSON configuration file as an argument:

```bash
./build/Plan2Scene cad/house_plan/Simple_House_Plan.json
```