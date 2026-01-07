#include <bitscrape/testing.hpp>

#include "bitscrape/dht/token_manager.hpp"

using namespace bitscrape::dht;
using namespace bitscrape::types;

TEST(TokenManagerTest, Constructor) {
  TokenManager manager;
}

TEST(TokenManagerTest, GenerateToken) {
  TokenManager manager;
  
  Endpoint ep(std::string("192.168.1.1"), 6881);
  
  DHTToken token = manager.generate_token(ep);
  
  EXPECT_TRUE(token.is_valid());
}

TEST(TokenManagerTest, GenerateTokenAsync) {
  TokenManager manager;
  
  Endpoint ep(std::string("192.168.1.1"), 6881);
  
  auto future = manager.generate_token_async(ep);
  DHTToken token = future.get();
  
  EXPECT_TRUE(token.is_valid());
}

TEST(TokenManagerTest, VerifyToken) {
  TokenManager manager;
  
  Endpoint ep(std::string("192.168.1.1"), 6881);
  
  DHTToken token = manager.generate_token(ep);
  
  EXPECT_TRUE(manager.verify_token(token, ep));
}

TEST(TokenManagerTest, VerifyTokenAsync) {
  TokenManager manager;
  
  Endpoint ep(std::string("192.168.1.1"), 6881);
  
  DHTToken token = manager.generate_token(ep);
  
  auto future = manager.verify_token_async(token, ep);
  EXPECT_TRUE(future.get());
}

TEST(TokenManagerTest, VerifyInvalidToken) {
  TokenManager manager;
  
  Endpoint ep(std::string("192.168.1.1"), 6881);
  
  DHTToken token;  // Invalid token
  
  EXPECT_FALSE(manager.verify_token(token, ep));
}

TEST(TokenManagerTest, VerifyTokenForDifferentEndpoint) {
  TokenManager manager;
  
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);
  
  DHTToken token = manager.generate_token(ep1);
  
  EXPECT_TRUE(manager.verify_token(token, ep1));
  EXPECT_FALSE(manager.verify_token(token, ep2));
}

TEST(TokenManagerTest, TokenRotation) {
  TokenManager manager;
  
  Endpoint ep(std::string("192.168.1.1"), 6881);
  
  DHTToken token1 = manager.generate_token(ep);
  
  // Simulate secret rotation
  for (int i = 0; i < 10; ++i) {
    manager.generate_token(ep);
  }
  
  // The original token should still be valid (previous secret)
  EXPECT_TRUE(manager.verify_token(token1, ep));
  
  // Generate a new token
  DHTToken token2 = manager.generate_token(ep);
  
  // Both tokens should be valid
  EXPECT_TRUE(manager.verify_token(token1, ep));
  EXPECT_TRUE(manager.verify_token(token2, ep));
}
