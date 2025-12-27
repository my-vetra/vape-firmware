
/**
 * @file LogBuffer.cpp
 * @brief Implementation of LogBuffer circular buffer for log messages.
 */

#include "LogBuffer.h"

// -----------------------------------------------------------------------------
// LogBuffer Method Implementations
// -----------------------------------------------------------------------------

LogBuffer& LogBuffer::instance() {
    static LogBuffer inst;
    return inst;
}

void LogBuffer::push(const std::string& line) {
    std::string s = line;
    if (s.size() > kMaxLineLen) s.resize(kMaxLineLen);
    if (q_.size() >= kCapacity) q_.pop_front();
    q_.push_back(std::move(s));
}

bool LogBuffer::pop(std::string& out) {
    if (q_.empty()) return false;
    out = std::move(q_.front());
    q_.pop_front();
    return true;
}

size_t LogBuffer::size() const {
    return q_.size();
}
