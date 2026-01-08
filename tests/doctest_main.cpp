// Provide doctest's main implementation for all unit test binaries
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

int main(int argc, char** argv) {
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int result = context.run();
    if (context.shouldExit()) {
        return result;
    }
    return result;
}
