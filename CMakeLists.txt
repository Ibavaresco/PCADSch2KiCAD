cmake_minimum_required(VERSION 3.12)
project(PCADSch2KiCAD)

set(CMAKE_C_STANDARD 11)

set(SOURCES
    KiCADOutputSchematic.c
    Lexic.c
    PCADEnums.c
    PCADOutputSchematic.c
    PCADParser.c
    PCADProcessSchematic.c
    Parser.c
    main.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

if (UNIX)
  link_libraries(m)
endif()
add_executable(PCADSch2KiCAD ${SOURCES})

if(UNIX AND NOT APPLE)
  install(TARGETS PCADSch2KiCAD DESTINATION /usr/bin)
endif()
