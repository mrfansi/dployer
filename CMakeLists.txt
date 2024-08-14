cmake_minimum_required(VERSION 3.15)
project(dployer C)

set(CMAKE_C_STANDARD 99)

# Set the installation directory to {HOME}/.config/dployer
set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.config/dployer")

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.c
    src/logger.c
    src/database.c
    src/repo.c
    src/deploy.c
    src/utils.c
    src/docker.c
)

# Link libraries
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

# Add the include directories and link libraries explicitly
include_directories(${JSONC_INCLUDE_DIRS})
link_directories(${JSONC_LIBRARY_DIRS})

add_executable(dployer ${SOURCES})

# Link against SQLite3 and JSON-C libraries
target_link_libraries(dployer SQLite::SQLite3 ${JSONC_LIBRARIES})

# Include directories for external libraries
target_include_directories(dployer PRIVATE ${JSONC_INCLUDE_DIRS})

# Install the binary to the {HOME}/.config/dployer/bin directory
install(TARGETS dployer DESTINATION bin)

# Install the configuration files to {HOME}/.config/dployer/config directory
install(DIRECTORY config/ DESTINATION config)