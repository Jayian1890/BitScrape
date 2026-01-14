# Web UI library - depends on types, network
file(GLOB_RECURSE WEB_SOURCES *.cpp)
add_library(web STATIC ${WEB_SOURCES})
target_include_directories(web PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(web PUBLIC types network)

# macOS menubar app
add_executable(bitscrape_web MACOSX_BUNDLE
    ${CMAKE_CURRENT_SOURCE_DIR}/menubar_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/mac/app_icon.png
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/mac/menubar_icon.png
    ${CMAKE_SOURCE_DIR}/public/index.html
    ${CMAKE_SOURCE_DIR}/public/styles.css
    ${CMAKE_SOURCE_DIR}/public/app.js
)

target_include_directories(bitscrape_web PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bitscrape_web PRIVATE web core "-framework AppKit")

set_target_properties(bitscrape_web PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    MACOSX_BUNDLE_BUNDLE_NAME "BitScrape"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.jaredterrance.bitscrape.web"
    MACOSX_BUNDLE_ICON_FILE "app_icon.png"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/resources/mac/app_icon.png PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/resources/mac/menubar_icon.png PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set_source_files_properties(${CMAKE_SOURCE_DIR}/public/index.html PROPERTIES MACOSX_PACKAGE_LOCATION Resources/public)
set_source_files_properties(${CMAKE_SOURCE_DIR}/public/styles.css PROPERTIES MACOSX_PACKAGE_LOCATION Resources/public)
set_source_files_properties(${CMAKE_SOURCE_DIR}/public/app.js PROPERTIES MACOSX_PACKAGE_LOCATION Resources/public)

