cmake_minimum_required(VERSION 3.19)
project(plctagt LANGUAGES CXX)

# --------------------------
# 1️⃣ Add libplctag as a subdirectory
# --------------------------
add_subdirectory(libplctag)

# --------------------------
# 2️⃣ Add your executable
# --------------------------
add_executable(plctagt
    main.cpp
    utility.h
    main.h
    plctags.cpp
    plctags.h

    utility.cpp
    list_tags.h
    list_tags.cpp
    gui.cpp
    exercise.cpp
    # Add other source files here if needed
)

# --------------------------
# 3️⃣ Link libplctag and ncurses
# --------------------------
find_package(Curses REQUIRED)
target_link_libraries(plctagt
    PRIVATE
        plctag_static
        ${CURSES_LIBRARIES}  # link ncurses
)

# --------------------------
# 4️⃣ Include headers
# --------------------------
target_include_directories(plctagt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libplctag/src/libplctag/lib
    ${CURSES_INCLUDE_DIR}  # ncurses headers
)

# --------------------------
# 5️⃣ Debug & ASan configuration (optional)
# --------------------------
target_compile_options(plctag_dyn PRIVATE $<$<CONFIG:Debug>:-g -O0>)

if(USE_SANITIZERS)
    message(STATUS "Enabling AddressSanitizer for plctagt and libplctag")
    target_compile_options(plctagt PRIVATE -fsanitize=address)
    target_compile_options(plctag_dyn PRIVATE -fsanitize=address)
    target_link_options(plctagt PRIVATE -fsanitize=address)
    target_link_options(plctag_dyn PRIVATE -fsanitize=address)
endif()

# --------------------------
# 6️⃣ Install settings
# --------------------------
include(GNUInstallDirs)
install(TARGETS plctagt
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
