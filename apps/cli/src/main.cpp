#include "bitscrape/types/node_id.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    std::cout << "BitScrape CLI" << std::endl;
    std::cout << "Version: 0.1.0" << std::endl;

    // Create a random node ID
    bitscrape::types::NodeID node_id = bitscrape::types::NodeID::secure_random();
    std::cout << "Random Node ID: " << node_id.to_hex() << std::endl;

    return 0;
}
