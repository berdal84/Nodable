require "rbconfig"
require 'json'

VERBOSE            = false
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

#---------------------------------------------------------------------------

module TargetType
  EXECUTABLE     = 0
  STATIC_LIBRARY = 1
  OBJECTS        = 2
end

Target = Struct.new(
    :name,
    :type,
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

def get_objects_from_targets( targets )
    objects = FileList[]
    targets.each do |other|
        objects |= get_objects(other);
    end
    objects
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

def get_objects( target )
    to_objects( target.sources )
end

def get_objects_to_link( target )
    objects = get_objects( target )
    target.link_library.each do |other_target|
        objects |= get_objects_to_link( other_target )
    end
    objects
end

def get_library_name( target )
    "#{BUILD_DIR}/lib/lib#{target.name.ext(".a")}"
end

def get_binary( target )
    path = "#{BIN_DIR}/#{target.name}"
    if PLATFORM_WEB
        path = path.ext("html")
    end
    path
end

def build_executable_binary( target )

    objects        = get_objects_to_link(target).join(" ")
    binary         = get_binary( target )
    defines        = target.defines.map{|d| "-D\"#{d}\"" }.join(" ")
    compiler_flags = target.compiler_flags.join(" ")
    linker_flags   = target.linker_flags.join(" ")

    FileUtils.mkdir_p File.dirname(binary)

    sh "#{$linker} #{compiler_flags} #{defines} -o #{binary} #{objects} #{linker_flags}", verbose: VERBOSE
end

def compile_file(src, target)

    # Generate stringified version of the flags
    # TODO: store this in a cache?
    includes     = target.includes.map{|path| "-I#{path}"}.join(" ")
    cxx_flags    = target.cxx_flags.join(" ")
    c_flags      = target.c_flags.join(" ")
    defines      = target.defines.map{|d| "-D\"#{d}\"" }.join(" ")
    compiler_flags = target.compiler_flags.join(" ")

    obj = src_to_obj( src )
    dep = src_to_dep( src )
    dep_flags = "-MD -MF#{dep}"

    
    if VERBOSE
        puts "-- obj: #{obj}"
        puts "-- dep: #{dep}"
    end

    FileUtils.mkdir_p File.dirname(obj)
    FileUtils.mkdir_p File.dirname(dep)

    if File.extname( src ) == ".cpp"
       compiler = "#{$cxx_compiler} #{compiler_flags} #{cxx_flags}"
    else
       compiler = "#{$c_compiler} #{compiler_flags} #{c_flags}"
    end

    sh "#{compiler} -c #{includes} #{defines} #{dep_flags} -o #{obj} #{src}", verbose: VERBOSE
end

def copy_asset(pattern) # pattern: "<source>:<destination>" (destination is optional)

    arr = pattern.split(':')

    # Source is required
    source = arr[0] or raise ("Wrong pattern: #{pattern}")

    # Destination is optional, by default we copy relative to repository root
    destination = "#{BIN_DIR}/#{arr[1] || source}";
    
    FileUtils.rm_f destination
    FileUtils.mkdir_p File.dirname(destination)
    FileUtils.copy_file( source, destination )
    puts "  Copy asset: #{source} => #{destination}"
end

def tasks_for_target(target)

    desc "Clean #{target.name}'s intermediate files"
    task :clean do
        FileUtils.rm_f get_objects(target)
    end

    if target.type == TargetType::EXECUTABLE
        desc "Run the #{target.name}"
        task :run => [ :build ] do

            if PLATFORM_DESKTOP
                sh "./#{get_binary(target)}"
            elsif PLATFORM_WEB
                sh "emrun --hostname #{HTTP_SERVER_HOSTNAME} --port #{HTTP_SERVER_PORT} #{get_binary(target)}"
            end
        end
    end

    desc "Clean and build target #{target.name}"
    task :rebuild => [:clean, :build]

    desc "Compile #{target.name}"
    task :build => get_binary(target) do
        target.assets.each do |pattern|
            copy_asset(pattern)
        end
    end

    file get_binary(target) => :link do
        case target.type
        when TargetType::EXECUTABLE
            puts "#{target.name} | Linking executable binary ..."
            build_executable_binary( target )
            puts "#{target.name} | Linking OK"

        when TargetType::STATIC_LIBRARY
            puts "#{target.name} | Linking static library ..."
            build_static_library( target )
            puts "#{target.name} | Linking OK"
        when TargetType::OBJECTS
            # nothing to go
        else
            raise "Unhandled case: #{target.type}"
        end 
    end    

    multitask :link => get_objects_to_link( target )

    get_objects(target).each_with_index do |obj, index|
        src = obj_to_src( obj, target )
        file obj => src do |task|
            puts "#{target.name} | Compiling #{src} ..."
            compile_file( src, target)
        end
    end
end
