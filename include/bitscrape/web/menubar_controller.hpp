#pragma once

#include <memory>
#include <string>

namespace bitscrape::web {

class HTTPServer;

// Simple controller to start and stop the embedded HTTP server for the menubar app.
class MenubarController {
public:
    MenubarController();
    ~MenubarController();

    // Starts the server on the given port (default 8080). Returns true on success.
    bool start_server(uint16_t port, const std::string& resource_path);

    // Stops the server if it is running.
    void stop_server();

    // Returns true if the server is currently running.
    bool is_running() const;

private:
    std::unique_ptr<HTTPServer> server_;
};

} // namespace bitscrape::web
