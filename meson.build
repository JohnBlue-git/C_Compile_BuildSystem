project(
  'cpp-interview-playground',
  ['c', 'cpp'],
  version: '1.0.0',
  default_options: [
    'c_std=c17',
    'cpp_std=c++20',
    'warning_level=3',
  ],
  meson_version: '>=1.1.0',
)

fs = import('fs')
cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')
threads_dep = dependency('threads')
dl_dep = cc.find_library('dl', required: false)

fs.copyfile('markdowns/C/examples/sample.txt', 'sample.txt')
fs.copyfile('markdowns/CXX/examples/library/data.txt', 'data.txt')

# Keep source, dependency, and test metadata together in each target row.
plugin_mangled = shared_library(
  'compile_plugin_mangled',
  'markdowns/Compile/examples/dlopen/add.cpp',
  install: false,
)
plugin_extern_c = shared_library(
  'compile_plugin_extern_c',
  'markdowns/Compile/examples/dlopen/add_c_linkage.cpp',
  install: false,
)

example_specs = [
  ['c_switch', files('markdowns/C/examples/switch.c'), 'c', [], [], true, []],
  ['c_pointer_2d', files('markdowns/C/examples/pointer_2d.c'), 'c', [], [], true, []],
  ['c_types', files('markdowns/C/examples/types.c'), 'c', [], [], true, []],
  ['c_union', files('markdowns/C/examples/union.c'), 'c', [], [], true, []],
  ['c_callbacks', files('markdowns/C/examples/callbacks.c'), 'c', [], [], true, []],
  ['c_copy', files('markdowns/C/examples/copy.c'), 'c', [], [], true, []],
  ['c_qualifiers', files('markdowns/C/examples/qualifiers.c'), 'c', [], [], true, []],
  ['c_preprocessor', files('markdowns/C/examples/preprocessor.c'), 'c', [], [], true, []],
  ['c_error_cleanup', files('markdowns/C/examples/error_cleanup.c'), 'c', [], [], true, []],
  ['c_signal_handler', files('markdowns/C/examples/signal_handler.c'), 'c', [], [], true, []],
  ['c_signal_timeout', files('markdowns/C/examples/signal_timeout.c'), 'c', [], [], true, []],
  ['c_file_io', files('markdowns/C/examples/file_io.c'), 'c', [], [], true, []],
  ['cxx_raii', files('markdowns/CXX/examples/core/raii.cpp'), 'cxx', [], [], true, []],
  ['cxx_template', files('markdowns/CXX/examples/core/template.cpp'), 'cxx', [], [], true, []],
  ['cxx_crtp', files('markdowns/CXX/examples/core/crtp.cpp'), 'cxx', [], [], true, []],
  ['cxx_crtp_benchmark', files('markdowns/CXX/examples/core/crtp_benchmark.cpp'), 'cxx', [], [], true, []],
  ['cxx_smart_pointers', files('markdowns/CXX/examples/core/smart_pointers.cpp'), 'cxx', [], [], true, []],
  ['cxx_weak_callback', files('markdowns/CXX/examples/core/weak_callback.cpp'), 'cxx', [], [], true, []],
  ['cxx_async_future', files('markdowns/CXX/examples/core/async_future.cpp'), 'cxx', [threads_dep], [], true, []],
  ['cxx_promise_future', files('markdowns/CXX/examples/core/promise_future.cpp'), 'cxx', [threads_dep], [], true, []],
  ['cxx_move_semantics', files('markdowns/CXX/examples/core/move_semantics.cpp'), 'cxx', [], [], true, []],
  ['cxx_rvo_nrvo', files('markdowns/CXX/examples/core/rvo_nrvo.cpp'), 'cxx', [], [], true, []],
  ['cxx_rvo_nrvo_no_elide', files('markdowns/CXX/examples/core/rvo_nrvo.cpp'), 'cxx', [], [], true, ['-fno-elide-constructors']],
  ['cxx_lambdas', files('markdowns/CXX/examples/core/lambdas.cpp'), 'cxx', [threads_dep], [], true, []],
  ['cxx_callbacks', files('markdowns/CXX/examples/core/callbacks.cpp'), 'cxx', [], [], true, []],
  ['cxx_casts', files('markdowns/CXX/examples/core/casts.cpp'), 'cxx', [], [], true, []],
  ['cxx_exceptions', files('markdowns/CXX/examples/core/exceptions.cpp'), 'cxx', [], [], true, []],
  ['cxx_optional', files('markdowns/CXX/examples/library/optional.cpp'), 'cxx', [], [], true, []],
  ['cxx_reference_wrapper', files('markdowns/CXX/examples/library/reference_wrapper.cpp'), 'cxx', [], [], true, []],
  ['cxx_emplace_back', files('markdowns/CXX/examples/library/emplace_back.cpp'), 'cxx', [], [], true, []],
  ['cxx_container_thread_safety', files('markdowns/CXX/examples/library/container_thread_safety.cpp'), 'cxx', [threads_dep], [], true, []],
  ['cxx_oop_encapsulation', files('markdowns/CXX/examples/library/oop/encapsulation.cpp'), 'cxx', [], [], true, []],
  ['cxx_oop_inheritance', files('markdowns/CXX/examples/library/oop/inheritance.cpp'), 'cxx', [], [], true, []],
  ['cxx_oop_polymorphism', files('markdowns/CXX/examples/library/oop/polymorphism.cpp'), 'cxx', [], [], true, []],
  ['cxx_oop_multi_inheritance', files('markdowns/CXX/examples/library/oop/multi_inheritance.cpp'), 'cxx', [], [], true, []],
  ['cxx_oop_singleton_cache', files('markdowns/CXX/examples/library/oop/singleton_cache.cpp'), 'cxx', [], [], true, []],
  ['cxx_stream_state', files('markdowns/CXX/examples/library/stream/stream_state.cpp'), 'cxx', [], [], true, []],
  ['cxx_stream_get_getline', files('markdowns/CXX/examples/library/stream/get_getline.cpp'), 'cxx', [], [], true, []],
  ['cxx_stream_input_buffer', files('markdowns/CXX/examples/library/stream/input_buffer.cpp'), 'cxx', [], [], true, []],
  ['cxx_stream_ofstream_put', files('markdowns/CXX/examples/library/stream/ofstream_put.cpp'), 'cxx', [], [], true, []],
  ['compile_mixed_c_cxx', files('markdowns/Compile/examples/mixed/main.cpp', 'markdowns/Compile/examples/mixed/add.c'), 'compile', [], [], true, []],
  ['compile_dlopen_loader', files('markdowns/Compile/examples/dlopen/loader.c'), 'compile', [dl_dep], [plugin_mangled.full_path(), plugin_extern_c.full_path()], true, []],
  ['memory_storage_duration', files('markdowns/Memory/examples/storage_duration.c'), 'memory', [], [], true, []],
  ['memory_object_lifetime', files('markdowns/Memory/examples/object_lifetime.cpp'), 'memory', [], [], true, []],
  ['memory_callback_lifetime', files('markdowns/Memory/examples/callback_lifetime.c'), 'memory', [], [], true, []],
  ['memory_bug_use_after_free', files('markdowns/Memory/examples/bugs/use_after_free.c'), 'memory', [], [], false, []],
  ['memory_bug_stack_use_after_return', files('markdowns/Memory/examples/bugs/stack_use_after_return.c'), 'memory', [], [], false, []],
  ['memory_bug_heap_overflow', files('markdowns/Memory/examples/bugs/heap_overflow.c'), 'memory', [], [], false, []],
  ['memory_bug_leak', files('markdowns/Memory/examples/bugs/leak.c'), 'memory', [], [], false, []],
]

nlohmann_dep = dependency('nlohmann_json', required: false)
if nlohmann_dep.found()
  example_specs += [
    ['cxx_json_nlohmann', files('markdowns/CXX/examples/library/json/nlohmann_basic.cpp'), 'cxx', [nlohmann_dep], [], true, []],
    ['cxx_json_nlohmann_pointer', files('markdowns/CXX/examples/library/json/nlohmann_pointer.cpp'), 'cxx', [nlohmann_dep], [], true, []],
  ]
else
  message('nlohmann_json not found: skipping cxx_json_nlohmann* examples')
endif

boost_json_dep = dependency('boost', modules: ['json'], required: false)
if boost_json_dep.found()
  example_specs += [
    ['cxx_json_boost', files('markdowns/CXX/examples/library/json/boost_basic.cpp'), 'cxx', [boost_json_dep], [], true, []],
    ['cxx_json_boost_pointer', files('markdowns/CXX/examples/library/json/boost_pointer.cpp'), 'cxx', [boost_json_dep], [], true, []],
  ]
else
  message('Boost.JSON not found: skipping cxx_json_boost* examples')
endif

foreach spec : example_specs
  target = executable(
    spec[0],
    spec[1],
    dependencies: spec[3],
    cpp_args: spec[6],
    install: false,
  )
  if spec[5]
    test(
      spec[0],
      target,
      args: spec[4],
      workdir: meson.current_build_dir(),
      timeout: 30,
      suite: spec[2],
    )
  endif
endforeach

# Compile/link labs are shell tests that also invoke CMake and binutils.
bash = find_program('bash')
foreach lab : ['pipeline', 'linkage', 'libraries', 'cmake_rpath']
  test(
    'compile_' + lab + '_lab',
    bash,
    args: [
      files('markdowns/Compile/examples/' + lab + '/lab.sh'),
      join_paths(meson.current_build_dir(), lab + '_lab'),
    ],
    workdir: meson.current_build_dir(),
    timeout: 60,
    suite: 'compile',
  )
endforeach

subdir('playground')
