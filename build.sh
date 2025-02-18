cmake . --preset default
cmake --build --preset default -j $(nproc)
build/default/async
