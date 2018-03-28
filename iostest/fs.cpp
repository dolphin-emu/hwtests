// Copyright 2018 leoetlino <leo@leolam.fr>
// Licensed under GPLv2

#include <array>
#include <cstring>

#include "common/CommonTypes.h"
#include "iostest/fs.h"
#include "iostest/ipc.h"

namespace fs {

enum
{
  ISFS_IOCTL_FORMAT = 1,
  ISFS_IOCTL_GETSTATS = 2,
  ISFS_IOCTL_CREATEDIR = 3,
  ISFS_IOCTLV_READDIR = 4,
  ISFS_IOCTL_SETATTR = 5,
  ISFS_IOCTL_GETATTR = 6,
  ISFS_IOCTL_DELETE = 7,
  ISFS_IOCTL_RENAME = 8,
  ISFS_IOCTL_CREATEFILE = 9,
  ISFS_IOCTL_SETFILEVERCTRL = 10,
  ISFS_IOCTL_GETFILESTATS = 11,
  ISFS_IOCTLV_GETUSAGE = 12,
  ISFS_IOCTL_SHUTDOWN = 13,
};

ipc::Result CreateFile(int fs_fd, const char* path, u8 owner, u8 group, u8 other, u8 attribute) {
  ISFSParams params{};
  std::strncpy(params.path.data(), path, params.path.size());
  params.owner_mode = owner;
  params.group_mode = group;
  params.other_mode = other;
  params.attribute = attribute;
  return ipc::Ioctl(fs_fd, ISFS_IOCTL_CREATEFILE, &params, sizeof(params), nullptr, 0);
}

ipc::Result DeleteFile(int fs_fd, const char* path) {
  std::array<char, 64> in_buffer;
  std::strncpy(in_buffer.data(), path, in_buffer.size());
  return ipc::Ioctl(fs_fd, ISFS_IOCTL_DELETE, in_buffer.data(), in_buffer.size(), nullptr, 0);
}

ipc::Result GetMetadata(int fs_fd, const char* path, ISFSParams* metadata) {
  std::array<char, 64> in_buffer;
  std::strncpy(in_buffer.data(), path, in_buffer.size());
  std::strncpy(metadata->path.data(), path, in_buffer.size());
  return ipc::Ioctl(fs_fd, ISFS_IOCTL_GETATTR, in_buffer.data(), in_buffer.size(), metadata,
                    sizeof(ISFSParams));
}
}  // namespace fs
