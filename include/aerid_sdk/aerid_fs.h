#ifndef AERID_FS_H
#define AERID_FS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>

    // Read a file into the provided buffer
    // Returns the number of bytes read, or -1 on error
    // All file operations are relative to the app's data directory
    int aerid_fs_read_file(const char *filename, uint8_t *buffer, size_t max_len);

    // Write data to a file
    // Returns the number of bytes written, or -1 on error
    // All file operations are relative to the app's data directory
    int aerid_fs_write_file(const char *filename, const uint8_t *data, size_t len);

    // Delete a file
    // All file operations are relative to the app's data directory
    bool aerid_fs_delete_file(const char *filename);

    // Move/rename a file
    // All file operations are relative to the app's data directory
    bool aerid_fs_move_file(const char *old_filename, const char *new_filename);

    // Check if a file exists
    // All file operations are relative to the app's data directory
    bool aerid_fs_file_exists(const char *filename);

    // Get the size of a file
    // Returns the size in bytes, or -1 on error
    // All file operations are relative to the app's data directory
    int64_t aerid_fs_get_file_size(const char *filename);

    // List files in the data directory
    // Fills the provided array with filenames (each max 64 bytes)
    // Returns the number of files found, or -1 on error
    int aerid_fs_list_files(char filenames[][64], size_t max_files);

    // Get the mount status
    bool aerid_fs_is_mounted();

    // Get the total and free space
    // Returns 0 on success, -1 on error
    int aerid_fs_get_space(uint64_t *total_bytes, uint64_t *free_bytes);

#ifdef __cplusplus
}
#endif
#endif