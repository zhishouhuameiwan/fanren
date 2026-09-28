-- 脚本不该能碰文件系统与进程环境：这些全局都应当是 nil。
assert(io == nil, "io 库不该开")
assert(os == nil, "os 库不该开")
assert(require == nil, "package 库不该开")
assert(dofile == nil, "dofile 应被摘掉")
assert(loadfile == nil, "loadfile 应被摘掉")
talk("a", "t.sandbox_ok")
