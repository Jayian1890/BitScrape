# GUI application - commented out until source files are available
# Create an empty main.cpp file for now
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp "
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << \"BitScrape GUI - Placeholder\" << std::endl;
    return 0;
}
")

add_executable(bitscrape_gui
    src/main.cpp
)

target_link_libraries(bitscrape_gui
    PRIVATE
        bitscrape::types
)

install(TARGETS bitscrape_gui
    RUNTIME DESTINATION bin
)
