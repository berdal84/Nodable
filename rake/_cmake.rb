
CMakeTarget = Struct.new(
    :name,
    :path,
    :config_flags,
    :install_flags,
    :build_flags,
    keyword_init: true # If the optional keyword_init keyword argument is set to true, .new takes keyword arguments instead of normal arguments.
)

def new_cmake_target(name, path)
    CMakeTarget.new(
        name: name,
        path: path,
        config_flags: "",
        install_flags: "",
        build_flags: ""
    )
end

def tasks_for_cmake_target( target )
    build_dir   = "#{BUILD_DIR}/cmake/#{target.name}"
    install_dir = BUILD_DIR # two folders (lib and include) will be created

    if BUILD_TYPE == "release"
        config = "Release"
    else
        config = "Debug"
    end

    task :rebuild => [:clean, :build]

    task :clean do
        puts "Cleaning ..."
        FileUtils.rm_rf build_dir
        puts "Cleaned"
    end

    task :build => build_dir

    file build_dir do
         # ensure folder exists
         if Dir.exist? build_dir
             FileUtils.rm_rf build_dir
         end
         FileUtils.mkdir_p build_dir

         config_cmd = "cmake -S #{target.path} -B #{build_dir}"

         if BUILD_OS_MACOS
            config_flags = "-DCMAKE_OSX_DEPLOYMENT_TARGET=#{MACOSX_VERSION_MIN}"
         elsif BUILD_OS_LINUX
            # none
         elsif BUILD_OS_MINGW
            config_flags = "-G \"Visual Studio 17 2022\" -A x64"
         end
         sh "#{config_cmd} #{config_flags} #{target.config_flags}" # configure
         sh "cmake --build #{build_dir} --config #{config} #{target.build_flags}"
    end

    task :install => :build do
        cmd = "cmake --install #{build_dir} --config #{config} --prefix #{install_dir}"
        if not install_dir and (BUILD_OS_LINUX or BUILD_OS_MACOS)
            sh "sudo #{cmd}" 
        else
            sh "#{cmd}"
        end
    end
end
