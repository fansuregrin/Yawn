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

    /**
     * @brief 初始化缓冲区
     * @param size 缓冲区初始大小
    */
    Buffer(size_type size = 1024);

    ~Buffer() = default;

    /**
     * @brief 缓冲区中可读的字节数（有效负载）
     * @return 字节数目
    */
    size_type readable_bytes() const;
    
    /**
     * @brief 缓冲区中可写的字节数
     * @return 字节数目
    */
    size_type writable_bytes() const;
    
    /**
     * @brief 缓冲区中前置的空闲字节数目
     * @return 字节数目
    */
    size_type prependable_bytes() const;
 
    /**
     * @brief 获取缓冲区中可读数据的开始位置
     * @return 缓冲区中可读数据的起始字节地址
    */
    const char * peek() const;
    
    /**
     * @brief 取回指定长度的数据
     * @param len 数据长度（单位为字节）
     */
    void retrieve(size_type len);

    /**
     * @brief 取回数据直到指定地址
     * @param end 结束地址
     */
    void retrieve_until(const char * end);
    
    /**
     * @brief 取回所有数据
     * 
     * 将可读数据的起始位置和可写数据的起始位置重置为 0
     */
    void retrieve_all();

    /**
     * @brief 取回所有数据，并作为字符串返回
     * @return 一个字符串
     */
    std::string retrieve_all_as_str();

    /**
     * @brief 获取可写空间的起始地址
     * @return 内存地址
     */
    char * begin_write();

    /**
     * @brief 获取可写空间的起始地址
     * @return 内存地址
     */
    const char * begin_write() const;

    const char * begin_write_const();

    void has_written(size_type sz);

    /**
     * @brief 确保能写入指定长度的数据
     * @param sz 数据长度（单位为字节）
     */
    void ensure_writable(size_type sz);

    /**
     * @brief 向缓冲区中追加数据
     * @param p `const char *` 类型，被追加的数据的起始地址
     * @param sz 数据长度（单位为字节）
     */
    void append(const char * p, size_type sz);
    
    /**
     * @brief 向缓冲区中追加数据
     * @param str 字符串
     */
    void append(const std::string &str);
    
    /**
     * @brief 向缓冲区中追加数据
     * @param p `const void *` 类型，被追加的数据的起始地址
     * @param sz 数据长度（单位为字节）
     */
    void append(const void * p, size_type sz);
    
    /**
     * @brief 向缓冲区中追加数据
     * @param other 另一个缓冲区
     */
    void append(const Buffer &other);

    /**
     * @brief 从指定文件描述符所标识的文件中读取数据
     * @param fd 文件描述符
     * @param saved_errno `int` 类型的指针，用于保存出错时的错误码
     * @return 读取的数据长度（单位为字节），长度小于零说明读取出错。
    */
    ssize_t read_fd(int fd, int * saved_errno);

    /**
     * @brief 向指定文件描述符所标识的文件中写入数据
     * @param fd 文件描述符
     * @param saved_errno `int` 类型的指针，用于保存出错时的错误码
     * @return 写入的数据长度（单位为字节），长度小于零说明写入出错。
    */
    ssize_t write_fd(int fd, int * saved_errno);

private:
    /**
     * @brief 获取缓冲区的起始地址
     * @return 起始地址
    */
    char * begin();

    /**
     * @brief 获取缓冲区的起始地址 (const 版本)
     * @return 起始地址
    */
    const char * begin() const;
    
    /**
     * @brief 腾出足够的写入空间
     * @param sz 需要腾出的写入空间大小（单位为字节）
     */
    void make_space(size_type sz);

    std::vector<char> buff;             // 缓冲区
    std::atomic<size_type> read_pos;    // 可读数据的起始位置
    std::atomic<size_type> write_pos;   // 可写数据的起始位置
};

#endif // BUFFER_H