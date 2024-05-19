/**
 * @file buffer.h
 * @author Fansure Grin
 * @date 2023-03-24
 * @brief head files for buffer
*/
#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <atomic>

/**
 * @brief 基于`std::vector<char>`实现的自动增长缓冲区
 * @code
 * +-------------------+------------------+------------------+
 * | prependable bytes |  readable bytes  |  writable bytes  |
 * |                   |     (CONTENT)    |                  |
 * +-------------------+------------------+------------------+
 * |                   |                  |                  |
 * 0       <=      read_pos     <=     write_pos    <=     size
 * @endcode
*/
class Buffer {
public:
    using size_type = std::vector<char>::size_type;

    Buffer(size_type size = 1024);
    ~Buffer() = default;

    size_type readable_bytes() const;
    size_type writable_bytes() const;
    size_type prependable_bytes() const;
 
    const char * peek() const;
    void retrieve(size_type len);
    void retrieve_until(const char * end);
    void retrieve_all();
    std::string retrieve_all_as_str();

    char * begin_write();
    const char * begin_write() const;
    const char * begin_write_const();
    void has_written(size_type sz);

    void ensure_writable(size_type sz);
    void append(const char * p, size_type sz);
    void append(const std::string &str);
    void append(const void * p, size_type sz);
    void append(const Buffer &other);

    ssize_t read_fd(int fd, int * saved_errno);
    ssize_t write_fd(int fd, int * saved_errno);
private:
    char * begin();
    const char * begin() const;
    void make_space(size_type sz);

    std::vector<char> buff;
    std::atomic<size_type> read_pos;
    std::atomic<size_type> write_pos;
};

#endif // BUFFER_H