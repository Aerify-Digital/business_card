#include "aerid_sdk/aerid_fs.h"
#include "sd_card.h"

extern volatile bool sd_mounted;

int aerid_fs_read_file(const char *filename, uint8_t *buffer, size_t max_len)
{
    if (!filename || !buffer || max_len == 0)
        return -1;

    sd_request_t req;
    req.op = SD_OP_READ;
    strncpy(req.filename, filename, sizeof(req.filename));
    req.buffer = buffer;
    req.length = max_len;
    req.done = xSemaphoreCreateBinary();

    xQueueSend(sdQueue, &req, portMAX_DELAY);
    xSemaphoreTake(req.done, portMAX_DELAY);
    vSemaphoreDelete(req.done);

    if (req.result)
    {
        return req.length;
    }
    else if (req.length == SD_ERR_TOO_LARGE)
    {
        return SD_ERR_TOO_LARGE;
    }
    return -1;
}

int aerid_fs_write_file(const char *filename, const uint8_t *data, size_t len)
{
    if (!filename || !data || len == 0)
        return -1;

    sd_request_t req;
    req.op = SD_OP_WRITE;
    strncpy(req.filename, filename, sizeof(req.filename));
    req.buffer = (uint8_t *)data;
    req.length = len;
    req.done = xSemaphoreCreateBinary();

    xQueueSend(sdQueue, &req, portMAX_DELAY);
    xSemaphoreTake(req.done, portMAX_DELAY);
    vSemaphoreDelete(req.done);
    if (req.result)
    {
        return len;
    }
    else if (req.length == SD_ERR_TOO_LARGE)
    {
        return SD_ERR_TOO_LARGE;
    }
    return -1;
}

int aerid_fs_append_file(const char *filename, const uint8_t *data, size_t len)
{
    if (!filename || !data || len == 0)
        return -1;

    sd_request_t req;
    req.op = SD_OP_APPEND;
    strncpy(req.filename, filename, sizeof(req.filename));
    req.buffer = (uint8_t *)data;
    req.length = len;
    req.done = xSemaphoreCreateBinary();

    xQueueSend(sdQueue, &req, portMAX_DELAY);
    xSemaphoreTake(req.done, portMAX_DELAY);
    vSemaphoreDelete(req.done);
    if (req.result)
    {
        return len;
    }
    else if (req.length == SD_ERR_TOO_LARGE)
    {
        return SD_ERR_TOO_LARGE;
    }
    return -1;
}

bool aerid_fs_delete_file(const char *filename)
{
    if (!filename)
        return false;

    if (!aerid_fs_is_mounted())
        return false;

    sd_request_t req;
    req.op = SD_OP_DELETE;
    strncpy(req.filename, filename, sizeof(req.filename));
    req.done = xSemaphoreCreateBinary();

    xQueueSend(sdQueue, &req, portMAX_DELAY);
    xSemaphoreTake(req.done, portMAX_DELAY);
    vSemaphoreDelete(req.done);

    if (req.result)
    {
        return true;
    }
    return false;
}

bool aerid_fs_move_file(const char *old_filename, const char *new_filename)
{
    if (!old_filename || !new_filename)
        return false;

    sd_request_t req;
    req.op = SD_OP_RENAME;
    strncpy(req.filename, old_filename, sizeof(req.filename));
    req.buffer = (uint8_t *)new_filename;
    req.done = xSemaphoreCreateBinary();

    xQueueSend(sdQueue, &req, portMAX_DELAY);
    xSemaphoreTake(req.done, portMAX_DELAY);
    vSemaphoreDelete(req.done);

    return req.result;
}

bool aerid_fs_file_exists(const char *filename)
{
    if (!filename)
        return false;
    // TODO: Implement file existence check

    return false;
}

int64_t aerid_fs_get_file_size(const char *filename)
{
    if (!filename)
        return -1;

    sd_request_t req;
    req.op = SD_OP_READ;
    strncpy(req.filename, filename, sizeof(req.filename));
    req.done = xSemaphoreCreateBinary();
    xQueueSend(sdQueue, &req, portMAX_DELAY);
    xSemaphoreTake(req.done, portMAX_DELAY);
    vSemaphoreDelete(req.done);

    if (req.result)
    {
        return req.length;
    }
    return -1;
}

int aerid_fs_list_files(char filenames[][64], size_t max_files)
{
    // TODO: Implement file listing functionality
    return -1;
}

bool aerid_fs_is_mounted()
{
    return sd_mounted;
}

int aerid_fs_get_space(uint64_t *total_bytes, uint64_t *free_bytes)
{
    // TODO: Implement space retrieval functionality
    return -1;
}