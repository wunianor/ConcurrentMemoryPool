# CMakeList.txt: 高并发内存池 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
#cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
#if (POLICY CMP0141)
#  cmake_policy(SET CMP0141 NEW)
#  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
#endif()

#project ("高并发内存池")

# 将源代码添加到此项目的可执行文件。
#add_executable (CMakeTarget "testCode.cpp" "ConcurrentMemoryPool.h" "FixedSizeMemoryPool.hpp"  "Common.h" "ThreadCache.h" "FragmentedMemoryList.h" "ThreadCache.cpp" "CentralCache.h" "CentralCache.cpp" "PageCache.h" "PageCache.cpp"  "ConcurrentMemoryPool.cpp")

#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET CMakeTarget PROPERTY CXX_STANDARD 20)
#endif()

# TODO: 如有需要，请添加测试并安装目标。


cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  # 性能分析模式下，禁用 EditAndContinue（仅保留 ProgramDatabase 格式）
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug>:EditAndContinue>,$<$<CONFIG:RelWithDebInfo,Release>:ProgramDatabase>>")
endif()

project ("高并发内存池")

# 第一步：先创建目标（必须在 target_xxx 之前）
add_executable (CMakeTarget 
  "testCode.cpp" 
  "ConcurrentMemoryPool.h" 
  "FixedSizeMemoryPool.hpp"  
  "Common.h" 
  "ThreadCache.h" 
  "FragmentedMemoryList.h" 
  "ThreadCache.cpp" 
  "CentralCache.h" 
  "CentralCache.cpp" 
  "PageCache.h" 
  "PageCache.cpp"  
  "ConcurrentMemoryPool.cpp"
 "SystemMemoryAlloc.h" "SystemMemoryAlloc.cpp" "CalculateTool.cpp" "SpanList.h" "SpanList.cpp"  "FragmentedMemoryList.cpp" )

# 第二步：配置编译选项（目标已创建，可安全使用 target_compile_options）
if (MSVC)
  # 公共编译选项（所有配置通用）
  target_compile_options(CMakeTarget PRIVATE 
    /std:c++20  # 强制 C++20
    /MP  # 多线程编译
    /W4  # 开启警告（可选）
  )

  # RelWithDebInfo/Release 配置（性能分析必须用这两个配置）
  target_compile_options(CMakeTarget PRIVATE 
    $<$<CONFIG:RelWithDebInfo,Release>:
      /O2  # 优化级别
      /Zi  # 生成调试信息（PDB）
      /Gy  # 函数级链接
    >
  )
endif()

# 第三步：配置链接选项（同样在目标创建之后）
if (MSVC)
  target_link_options(CMakeTarget PRIVATE 
    # 所有配置通用：禁用增量链接
    /INCREMENTAL:NO
    # RelWithDebInfo/Release 配置：性能分析关键开关
    $<$<CONFIG:RelWithDebInfo,Release>:
      /PROFILE  # 启用性能分析支持（必须）
      /DEBUG  # 保留调试信息（PDB）
      /OPT:REF  # 移除未使用代码
      /OPT:ICF  # 合并重复函数
      /LTCG  # 链接时优化（可选）
    >
  )
endif()

# 第四步：设置 C++ 标准（目标已存在）
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CMakeTarget PROPERTY CXX_STANDARD 20)
  set_property(TARGET CMakeTarget PROPERTY CXX_STANDARD_REQUIRED ON)
  set_property(TARGET CMakeTarget PROPERTY CXX_EXTENSIONS OFF)  # 禁用编译器扩展
endif()