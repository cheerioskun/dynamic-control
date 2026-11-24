define dso_local void @netbench_memory_work(ptr nofree noundef readonly captures(none) %workers, i64 noundef %iterations) #1 {
entry:
  %destination = load atomic ptr, ptr @netbench_memory_work.dc_target acquire, align 8
  musttail call void %destination(ptr nofree noundef readonly captures(none) %workers, i64 noundef %iterations) #8
  ret void
}

define internal void @netbench_memory_work.dc_guard(ptr nofree noundef readonly captures(none) %0, i64 noundef %1) #8 {
entry:
  call void @dc_acquire(ptr @__dc_functions)
  call void @netbench_memory_work.dc_body(ptr nofree noundef readonly captures(none) %0, i64 noundef %1) #7
  call void @dc_release(ptr @__dc_functions)
  ret void
}
