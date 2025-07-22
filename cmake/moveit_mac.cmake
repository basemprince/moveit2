set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_OSX_ARCHITECTURES
    "arm64"
    CACHE STRING "" FORCE)

# Use libc++ by default
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")

# Force C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
