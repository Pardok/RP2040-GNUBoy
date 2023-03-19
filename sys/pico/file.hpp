#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

enum OpenMode {
  /// Open file for reading
  read  = 1 << 0,
  /// Open file for writing
  write = 1 << 1,
  /// Copy file to the temp area in flash for faster access
  cached = 1 << 2
};

enum FileFlags {
/// Is a directory
directory = 1
};

struct FileInfo {
  /// Name of the file
  std::string name;
  /// Flags (see ::FileFlags)
  int flags;
  /// Size of the file
  uint32_t size;
};

void init_fs();

bool get_files_open();
void close_open_files();

#ifdef __cplusplus
extern "C" {
#endif
void *open_file(const std::string &file, int mode);
int32_t read_file(void *fh, uint32_t offset, uint32_t length, char *buffer);
int32_t write_file(void *fh, uint32_t offset, uint32_t length, const char *buffer);
int32_t close_file(void *fh);
uint32_t get_file_length(void *fh);
#ifdef __cplusplus
}
#endif

void list_files(const std::string &path, std::function<void(FileInfo &)> callback);
bool file_exists(const std::string &path);
bool directory_exists(const std::string &path);
bool create_directory(const std::string &path);
bool rename_file(const std::string &old_name, const std::string &new_name);
bool remove_file(const std::string &path);
const char *get_save_path();
