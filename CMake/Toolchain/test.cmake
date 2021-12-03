
get_filename_component(windows_kits_dir
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE)

set(CMAKE_RC_COMPILER "C:/Program Files/LLVM/bin/llvm-rc.exe")

set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang-cl.exe")