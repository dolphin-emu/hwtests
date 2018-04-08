add_hwtest(MODULE iostest TEST ipc_timing FILES ipc_timing.cpp ipc.cpp)
add_hwtest(MODULE iostest TEST fs_timing FILES fs_timing.cpp ipc.cpp fs.cpp)
