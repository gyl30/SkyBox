set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)


find_package(Python COMPONENTS Interpreter)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF CACHE BOOL "Silence fetch content" FORCE)

set(QT_VERSION Qt5 CACHE STRING "The Qt version framework to use (Qt5 or Qt6).")
set(BREEZE_EXTENSIONS all CACHE STRING "The extensions to include in our stylesheets.")
set(BREEZE_STYLES all CACHE STRING "The styles to include in our stylesheets.")

FetchContent_Declare(
  breeze_stylesheets
  GIT_REPOSITORY https://github.com/Alexhuszagh/BreezeStyleSheets.git
  GIT_TAG origin/main
  GIT_PROGRESS ON
  GIT_SHALLOW 1
  USES_TERMINAL_DOWNLOAD TRUE)

FetchContent_GetProperties(breeze_stylesheets)
if(NOT breeze_stylesheets_POPULATED)
  FetchContent_Populate(breeze_stylesheets)

  add_library(breeze STATIC "${breeze_stylesheets_SOURCE_DIR}/dist/breeze.qrc")

  add_custom_target(
    run_python_breeze ALL
    COMMAND ${Python_EXECUTABLE} configure.py --extensions=${BREEZE_EXTENSIONS}
            --styles=${BREEZE_STYLES} --resource breeze.qrc
    WORKING_DIRECTORY ${breeze_stylesheets_SOURCE_DIR}
    BYPRODUCTS "${breeze_stylesheets_SOURCE_DIR}/dist/breeze.qrc"
    COMMENT "Generating themes")

  add_dependencies(breeze run_python_breeze)
endif()

aux_source_directory(${PROJECT_SOURCE_DIR}/gui CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/file CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/crypt CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/log CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/net CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/protocol CLI_GUI_SOURCE_FILE)

add_executable(gclient 
    main.cpp 
    widget.cpp
    table_view.cpp
    table_model.cpp
    table_delegate.cpp
    files_widget.cpp
    ${CLI_GUI_SOURCE_FILE})

target_link_libraries(gclient ${LINK_LIBS} Qt${QT_VERSION_MAJOR}::Widgets breeze)
