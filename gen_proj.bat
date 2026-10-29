if not exist "./build" mkdir "./build"
cd build
cmake -G "Visual Studio 18 2026" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..