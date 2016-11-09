project(RebootInto)
find_package(TCLAP REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)
find_package(Nowide REQUIRED)

include_directories(
    ${INCLUDE_PATH}
    ${TCLAP_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${Nowide_INCLUDE_DIRS}
)

set(SOURCE_FILES
    main.cpp
)
source_group("" FILES ${SOURCE_FILES})

if(MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Ox /Os")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /level='requireAdministrator'")
endif()

add_executable(RebootInto ${SOURCE_FILES})