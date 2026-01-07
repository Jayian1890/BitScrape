#include "bitscrape/network/socket.hpp"

#include <bitscrape/testing.hpp>

namespace bitscrape::network::test {

TEST(SocketTest, Construction) {
    // Test UDP socket construction
    EXPECT_NO_THROW({
        Socket udp_socket(SocketType::UDP);
        EXPECT_TRUE(udp_socket.is_valid());
        EXPECT_EQ(udp_socket.type(), SocketType::UDP);
    });

    // Test TCP socket construction
    EXPECT_NO_THROW({
        Socket tcp_socket(SocketType::TCP);
        EXPECT_TRUE(tcp_socket.is_valid());
        EXPECT_EQ(tcp_socket.type(), SocketType::TCP);
    });
}

TEST(SocketTest, MoveConstruction) {
    Socket socket1(SocketType::UDP);
    EXPECT_TRUE(socket1.is_valid());

    Socket socket2(std::move(socket1));
    EXPECT_TRUE(socket2.is_valid());
    EXPECT_FALSE(socket1.is_valid());
}

TEST(SocketTest, MoveAssignment) {
    Socket socket1(SocketType::UDP);
    EXPECT_TRUE(socket1.is_valid());

    Socket socket2(SocketType::TCP);
    EXPECT_TRUE(socket2.is_valid());

    socket2 = std::move(socket1);
    EXPECT_TRUE(socket2.is_valid());
    EXPECT_EQ(socket2.type(), SocketType::UDP);
    EXPECT_FALSE(socket1.is_valid());
}

TEST(SocketTest, Close) {
    Socket socket(SocketType::UDP);
    EXPECT_TRUE(socket.is_valid());

    socket.close();
    EXPECT_FALSE(socket.is_valid());
}

TEST(SocketTest, Bind) {
    Socket socket(SocketType::UDP);
    EXPECT_TRUE(socket.is_valid());

    // Bind to any address on a high port (unlikely to be in use)
    EXPECT_TRUE(socket.bind(50000));
}

TEST(SocketTest, BindWithAddress) {
    Socket socket(SocketType::UDP);
    EXPECT_TRUE(socket.is_valid());

    // Bind to localhost on a high port (unlikely to be in use)
    EXPECT_TRUE(socket.bind("127.0.0.1", 50001));
}

TEST(SocketTest, SetNonBlocking) {
    Socket socket(SocketType::UDP);
    EXPECT_TRUE(socket.is_valid());

    EXPECT_TRUE(socket.set_non_blocking(true));
    EXPECT_TRUE(socket.set_non_blocking(false));
}

TEST(SocketTest, SetBufferSizes) {
    Socket socket(SocketType::UDP);
    EXPECT_TRUE(socket.is_valid());

    EXPECT_TRUE(socket.set_receive_buffer_size(8192));
    EXPECT_TRUE(socket.set_send_buffer_size(8192));
}

TEST(SocketTest, SetTimeouts) {
    Socket socket(SocketType::UDP);
    EXPECT_TRUE(socket.is_valid());

    EXPECT_TRUE(socket.set_receive_timeout(1000));
    EXPECT_TRUE(socket.set_send_timeout(1000));
}

} // namespace bitscrape::network::test
