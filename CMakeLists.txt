cmake_minimum_required(VERSION 3.16)
include(GNUInstallDirs)

set(PROJ KanoopTorrentQt)
project(${PROJ})

find_package(Qt6 REQUIRED COMPONENTS Network)
qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(KANOOP_TORRENT_VERSION "0.1.0")

file(GLOB_RECURSE PROJ_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc"
)

qt_add_library(${PROJ} ${PROJ_SOURCES})

target_compile_options(${PROJ} PRIVATE -Wextra -Wall -Werror)

set_target_properties(${PROJ} PROPERTIES VERSION "${KANOOP_TORRENT_VERSION}")
set_target_properties(${PROJ} PROPERTIES SOVERSION 1)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libKanoopTorrentQt.pc.in" "libKanoopTorrentQt.pc" @ONLY)

target_include_directories(${PROJ} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJ} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/Kanoop/torrent)
target_include_directories(${PROJ} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(Qt6 REQUIRED COMPONENTS Core Network)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBTORRENT REQUIRED IMPORTED_TARGET libtorrent-rasterbar)

# Link Qt and libtorrent (always required)
target_link_libraries(${PROJ} PRIVATE
    Qt6::Core
    Qt6::Network
    PkgConfig::LIBTORRENT
)

# KanoopCommonQt and KanoopProtocolQt — resolve as targets (superproject)
# or fall back to include-path-only (standalone CI build)
if(TARGET KanoopCommonQt)
    target_link_libraries(${PROJ} PRIVATE KanoopCommonQt)
else()
    find_library(KANOOP_COMMON_LIB KanoopCommonQt)
    if(KANOOP_COMMON_LIB)
        target_link_libraries(${PROJ} PRIVATE ${KANOOP_COMMON_LIB})
    endif()
endif()

if(TARGET KanoopProtocolQt)
    target_link_libraries(${PROJ} PRIVATE KanoopProtocolQt)
else()
    find_library(KANOOP_PROTOCOL_LIB KanoopProtocolQt)
    if(KANOOP_PROTOCOL_LIB)
        target_link_libraries(${PROJ} PRIVATE ${KANOOP_PROTOCOL_LIB})
    endif()
endif()

# Resolve include directories — check superproject sibling, source sibling, and KANOOP_DEPS_DIR
get_directory_property(PARENT_DIR PARENT_DIRECTORY)
foreach(DEP KanoopCommonQt KanoopProtocolQt)
    if(EXISTS "${PARENT_DIR}/${DEP}/include")
        target_include_directories(${PROJ} PRIVATE ${PARENT_DIR}/${DEP}/include)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../${DEP}/include")
        target_include_directories(${PROJ} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../${DEP}/include)
    elseif(KANOOP_DEPS_DIR AND EXISTS "${KANOOP_DEPS_DIR}/${DEP}/include")
        target_include_directories(${PROJ} PRIVATE ${KANOOP_DEPS_DIR}/${DEP}/include)
    endif()
endforeach()

add_compile_definitions(KANOOP_TORRENT_LIBRARY)
add_compile_definitions(QT_DEPRECATED_WARNINGS)
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)

if (DEFINED $ENV{KYBER_OUTPUT_PREFIX})
    set(KANOOP_INSTALL_PREFIX "$ENV{KYBER_OUTPUT_PREFIX}/")
endif()

install(TARGETS ${PROJ} LIBRARY DESTINATION "${KANOOP_INSTALL_PREFIX}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/Kanoop" DESTINATION "${KANOOP_INSTALL_PREFIX}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.h")

find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(Doxygen_FOUND)
    add_custom_target(doc_KanoopTorrentQt
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating KanoopTorrentQt API documentation"
        VERBATIM
    )
endif()
