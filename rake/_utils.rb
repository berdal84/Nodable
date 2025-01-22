require "rbconfig"
require 'json'
require 'date' # To add date in .clang export

VERBOSE            = ENV["VERBOSE"] == "1"
BUILD_OS           = RbConfig::CONFIG['build_os']
HOST_OS            = RbConfig::CONFIG['host_os']
PLATFORM           = (ENV["PLATFORM"] || "desktop").downcase
PLATFORM_DESKTOP   = PLATFORM == "desktop"
PLATFORM_WEB       = PLATFORM == "web"
BUILD_TYPE         = (ENV["BUILD_TYPE"] || "release").downcase
BUILD_TYPE_RELEASE = BUILD_TYPE == "release"
BUILD_TYPE_DEBUG   = BUILD_TYPE != "release"
BUILD_DIR          = ENV["BUILD_DIR"] || "build-#{PLATFORM}-#{BUILD_TYPE}"
OBJ_DIR            = "#{BUILD_DIR}/obj"
DEP_DIR            = "#{BUILD_DIR}/dep"
BIN_DIR            = "#{BUILD_DIR}/bin"
BUILD_OS_LINUX     = BUILD_OS.include?("linux")
GITHUB_ACTIONS     = ENV["GITHUB_ACTIONS"]
HTTP_SERVER_HOSTNAME = "0.0.0.0"
HTTP_SERVER_PORT     = "8000"
HTTP_SERVER_URL      = "http://#{HTTP_SERVER_HOSTNAME}:#{HTTP_SERVER_PORT}/"

if VERBOSE
    system "echo Ruby version: && ruby -v"
    puts "BUILD_OS_LINUX:     #{BUILD_OS_LINUX}"
    puts "PLATFORM:           #{PLATFORM}"
    puts "BUILD_TYPE_RELEASE: #{BUILD_TYPE_RELEASE}"
    puts "BUILD_TYPE_DEBUG:   #{BUILD_TYPE_DEBUG}"
    puts "HTTP_SERVER_HOSTNAME: #{HTTP_SERVER_HOSTNAME}"
    puts "HTTP_SERVER_PORT:     #{HTTP_SERVER_PORT}"
end

if PLATFORM_DESKTOP
    $c_compiler   = "clang-15"
    $cxx_compiler = "clang++-15"
    $linker       = "clang++-15"
elsif PLATFORM_WEB
    $c_compiler   = "emcc"
    $cxx_compiler = "emcc"
    $linker       = "emcc"
else
    raise "Unexpected platform!"
end

TARGET_TYPE_OBJECTS    = "objects"
TARGET_TYPE_EXECUTABLE = "executable"

#---------------------------------------------------------------------------

Target = Struct.new(
    :name,
    :type, # TARGET_TYPE_XXX
    :sources, # list of .c|.cpp files
    :link_library, # list of other targets to link with (their compiled *.o will be linked)
    :includes, # list of path dir to include
    :defines,
    :compiler_flags,
    :c_flags,
    :cxx_flags,
    :linker_flags,
    :assets, # List of patterns like: "<source>" or "<source>:<destination>"
    keyword_init: true # If the optional keyword_init keyword argument is set to true, .new takes keyword arguments instead of normal arguments.
)

def new_empty_target(name, type)
    target = Target.new
    target.name = name
    target.type = type
    target.sources  = FileList[]
    target.link_library = []
    target.includes = FileList[]
    target.c_flags  = []
    target.cxx_flags = []
    target.linker_flags = []
    target.assets = FileList[]
    target.defines = []
    target.compiler_flags = []
    target.link_library = []
    target
end

def src_to_obj( obj )
    "#{OBJ_DIR}/#{ obj.ext(".o")}"
end

def src_to_dep( src )
    "#{DEP_DIR}/#{src.ext(".d")}"
end

def obj_to_src( obj, _target)
    stem = obj.sub("#{OBJ_DIR}/", "").ext("")
    _target.sources.detect{|src| src.ext("") == stem } or raise "unable to find #{obj}'s source (stem: #{stem})"
end

def to_objects( sources )
    sources.map{|src| src_to_obj(src) };
end

def get_self_objects( target )
    to_objects( target.sources )
end

def get_objects__incl_deps( target )
    
    # Take this target's objects
    objects = get_self_objects( target )

    # Append dependencies's objects
    target.link_library.each do |other_target|
        objects |= get_objects__incl_deps( other_target )
    end

    objects
end

def get_binary_path( target )
    path = "#{BIN_DIR}/#{target.name}"
    if PLATFORM_WEB
        path = path.ext("html")
    end
    path
end

def compile_file(src, target)
    
    # Ensure target folders exist
    FileUtils.mkdir_p File.dirname( src_to_obj( src ) )
    FileUtils.mkdir_p File.dirname( src_to_dep( src ) )

    # Get the command as string
    # build a command from an array of strings, similarly to docker
    # [<program>, <arg1>, <arg2>, ...]

    is_cpp = File.extname( src ) == ".cpp"

    cmd = [is_cpp ? $cxx_compiler : $c_compiler]
    cmd += target.compiler_flags
    cmd += is_cpp ? target.cxx_flags : target.c_flags
    cmd += ['-c'] # no linking
    cmd += format_includes(target)
    cmd += format_defines(target)
    cmd += get_dependency_flags(src)
    cmd += ["-o", src_to_obj( src ), src]

    # Run the command
    sh "#{cmd.join(" ")}", verbose: VERBOSE
end

def link_binary( target )

    if (target.type != TARGET_TYPE_EXECUTABLE)
        raise "Target type is expected to be: '#{TARGET_TYPE_EXECUTABLE}', actual: #{target.type}"
    end

    binary_path    = get_binary_path(target)
    objects        = get_objects__incl_deps(target).join(" ")
    defines        = target.defines.map{|d| "-D\"#{d}\"" }.join(" ")
    compiler_flags = target.compiler_flags.join(" ")
    linker_flags   = target.linker_flags.join(" ")

    FileUtils.mkdir_p File.dirname(binary_path)

    sh "#{$linker} #{compiler_flags} #{defines} -o #{binary_path} #{objects} #{linker_flags}", verbose: VERBOSE
end

def get_clangd_yaml(target)
# Note:
#   I couln't get the formatting I wanted (no double quotes on strings)
#   That's why I do manually, but it is simple.
#
# Format documentation: https://clangd.llvm.org/config#compileflags
#
    cmd = get_compile_file_command("fake.cpp", target)
    flags = cmd.split(" ") # remove program name

    result = "#
# .clang file generated for target #{target.name}, #{DateTime.now}
# !This file has been generated by nodable's rake script
# Do not modify it manually, edit _utils.rb instead!
#
CompileFlags:
  Add: [#{flags.join(",")}]
  Compiler: #{$cxx_compiler}

"
    result    
end

def format_defines(target)
    target.defines.map{|d| "-D\"#{d}\"" }
end

def format_includes(target)
    target.includes.map{|f| "-I#{File.absolute_path(f)}"}
end

def get_dependency_flags(source)
    ["-MD", "-MF#{src_to_dep(source)}"]
end

def get_assets_src(target)
    assets = []
    target.assets.each do |pattern|
        source, destination = split_asset_pattern(pattern)
        assets.append( source )
    end
    assets
end

def get_assets_dest(target)
    assets = []
    target.assets.each do |pattern|
        source, destination =  split_asset_pattern(pattern)
        assets.append( destination )
    end
    assets
end

def split_asset_pattern(pattern)  # pattern: "<source>:<destination>" (destination is optional)
    arr = pattern.split(':')

    # Source is required
    source = arr[0] or raise ("Wrong pattern: #{pattern}")

    # Destination is optional, by default we copy relative to repository root
    destination = "#{BIN_DIR}/#{arr[1] || source}";

    [source, destination]
end

def copy_asset(source, destination)
    FileUtils.rm_f destination
    FileUtils.mkdir_p File.dirname(destination)
    FileUtils.copy_file( source, destination )
    puts "  Copy asset: #{source} => #{destination}"
end

def tasks_for_target(target)

    objects = get_self_objects(target)
    objects_with_deps = get_objects__incl_deps(target)

    desc "Clean intermediate files (#{target.name})"
    task :clean do
        FileUtils.rm_f objects
    end

    desc "Clean intermediate files including dependencies (#{target.name})"
    task :clean_all do
        FileUtils.rm_f objects_with_deps
    end

    desc "Clean and build (#{target.name})"
    task :rebuild => [:clean, :build]

    desc "Clean and build all (#{target.name})"
    task :rebuild_all => [:clean_all, :build]

    if target.type == TARGET_TYPE_OBJECTS

        desc "Compile individual objects (#{target.name})"
        multitask :build => objects_with_deps do
            puts "#{target.name} | Build DONE"
        end
        
    elsif target.type == TARGET_TYPE_EXECUTABLE

        desc "Compile and link binary (#{target.name})"
        task :build => [:link, :copy_assets] do
            puts "#{target.name} | Build DONE"
        end

        # assets
        assets_src = get_assets_src(target)
        assets_dst = get_assets_dest(target)
        multitask :copy_assets => assets_dst do 
            puts "Assets copy DONE"
        end
        target.assets.each_with_index do |_, i|
            file assets_dst[i] => assets_src[i] do
                copy_asset(assets_src[i], assets_dst[i])
            end
        end
        
        task :link => get_binary_path(target)

        file get_binary_path(target) => :compile_objects do
            puts "#{target.name} | Linking '#{get_binary_path(target)}'..."
            link_binary(target)
            puts "#{target.name} | Linking '#{get_binary_path(target)}' DONE"
        end

        multitask :compile_objects => objects_with_deps

        desc "Run #{target.name}"
        task :run => :build do

            if PLATFORM_DESKTOP
                sh "./#{get_binary_path(target)}"
            elsif PLATFORM_WEB
                sh "emrun --hostname #{HTTP_SERVER_HOSTNAME} --port #{HTTP_SERVER_PORT} #{get_binary_path(target)}"
            end
        end
    end

    # Add a task per object to build (dependencies excluded)
    objects.each_with_index do |obj, index|
        src = obj_to_src( obj, target )
        file obj => src do |task|
            puts "#{target.name} | Compiling #{src} ..."
            compile_file( src, target)
        end
    end

    # Return the command to compile a given file
    #
    #   File don't need to exists since nothing run,
    #   we simplify build the command as string and return it via stdout.
    #
    task :compile_command, [:filename] do |t, args|
        filename = args[:filename] || "file.cpp"
        puts get_compile_file_command( filename, target)
    end

    # Generates a clangd yaml file from target
    #   Save it as .clangd to enable clangd to analyze your code.
    task :clangd do

        file_path = ".clangd"
        puts "Generating #{file_path} yaml file ..."

        data = get_clangd_yaml(target)

        if ( File.exists?(file_path))
            puts "  #{file_path} already exists, removing existing file"
            File.delete(file_path)
        end
        puts "  writing #{file_path} ..."
        File.write(file_path, data)
        puts "  #{file_path} updated with the following data:"
        puts data
    end
end
