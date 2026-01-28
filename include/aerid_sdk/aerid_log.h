#ifndef AERID_LOG_H
#define AERID_LOG_H
#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        AERID_LOG_DEBUG,
        AERID_LOG_INFO,
        AERID_LOG_WARN,
        AERID_LOG_ERROR
    } aerid_log_level_t;

    // Log a message at the specified log level
    void aerid_log(aerid_log_level_t level, const char *message);

    // Log a formatted message at the specified log level
    void aerid_log_ex(aerid_log_level_t level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif