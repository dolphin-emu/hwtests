// Copyright 2018 leoetlino <leo@leolam.fr>
// Licensed under GPLv2

#pragma once

#include "Common/CommonTypes.h"
#include "iostest/ipc.h"

namespace fs {

#pragma pack(push, 1)
struct ISFSParams
{
  u32 uid;
  u16 gid;
  std::array<char, 64> path;
  u8 owner_mode;
  u8 group_mode;
  u8 other_mode;
  u8 attribute;
};
#pragma pack(pop)

ipc::Result CreateFile(int fs_fd, const char* path, u8 owner, u8 group, u8 other, u8 attribute);
ipc::Result DeleteFile(int fs_fd, const char* path);
ipc::Result GetMetadata(int fs_fd, const char* path, ISFSParams* metadata);

}  // namespace fs
