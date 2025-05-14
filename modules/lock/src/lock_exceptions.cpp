#include "bitscrape/lock/lock_exceptions.hpp"

namespace bitscrape::lock {

DeadlockException::DeadlockException(const std::string& message, const std::vector<uint64_t>& resources)
    : std::runtime_error(message), resources_(resources) {
}

const std::vector<uint64_t>& DeadlockException::get_resources() const {
    return resources_;
}

TimeoutException::TimeoutException(const std::string& message, uint64_t resource_id)
    : std::runtime_error(message), resource_id_(resource_id) {
}

uint64_t TimeoutException::get_resource() const {
    return resource_id_;
}

LockOperationException::LockOperationException(const std::string& message)
    : std::runtime_error(message) {
}

} // namespace bitscrape::lock
