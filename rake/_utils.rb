require "rbconfig"
require 'json'

VERBOSE            = false
C_COMPILER         = "clang-15"
CXX_COMPILER       = "clang++-15"
BUILD_OS           = RbConfig::CONFIG['build_os']
HOST_OS            = RbConfig::CONFIG['host_os']
TARGET             = (ENV["TARGET"] || "desktop").downcase
BUILD_TYPE         = (ENV["BUILD_TYPE"] || "release").downcase
BUILD_TYPE_RELEASE = BUILD_TYPE == "release"
BUILD_TYPE_DEBUG   = BUILD_TYPE != "release"
BUILD_DIR          = ENV["BUILD_DIR"] || "build-#{TARGET}-#{BUILD_TYPE}"
OBJ_DIR            = "#{BUILD_DIR}/obj"
DEP_DIR            = "#{BUILD_DIR}/dep"
BIN_DIR            = "#{BUILD_DIR}/bin"
BUILD_OS_LINUX     = BUILD_OS.include?("linux")
GITHUB_ACTIONS     = ENV["GITHUB_ACTIONS"]

if TARGET != "desktop" && TARGET != "web"
    raise "Unexpected value for TARGET, use desktop|web";
end

if VERBOSE
    system "echo Ruby version: && ruby -v"
    puts "BUILD_OS_LINUX:     #{BUILD_OS_LINUX}"
    puts "TARGET:             #{TARGET}"
    puts "COMPILER_FOUND:     #{COMPILER_FOUND}"
    puts "BUILD_TYPE_RELEASE: #{BUILD_TYPE_RELEASE}"
    puts "BUILD_TYPE_DEBUG:   #{BUILD_TYPE_DEBUG}"
end

COMPILER_FOUND = system "#{C_COMPILER} --version" || false
if not COMPILER_FOUND
    raise "Unable to find #{C_COMPILER} from path."
elsif (not BUILD_OS_LINUX)
    raise "Unable to determine the operating system"
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
    :asset_folder_path,
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
    target.asset_folder_path = nil
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

def copy_assets_to( dest_dir, target )
    from = target.asset_folder_path
    to   = "#{dest_dir}/#{target.asset_folder_path}"
    if Dir.exist?(to)
        puts "Skip assets copy (#{to} already exists)"
    else
        puts "#{target.name} Copying assets to #{to} ..."
        FileUtils.mkdir_p to
        FileUtils.copy_entry( from, to )
        puts "#{target.name} Assets copy OK"
    end    
end

def get_library_name( target )
    "#{BUILD_DIR}/lib/lib#{target.name.ext(".a")}"
end

def build_static_library( target )

    objects        = get_objects_to_link( target ).join(" ")
    binary         = get_library_name( target )
    linker_flags   = target.linker_flags.join(" ")

    FileUtils.mkdir_p File.dirname(binary)
    sh "llvm-ar r #{binary} #{objects}", verbose: VERBOSE
end

def get_binary( target )
    "#{BIN_DIR}/#{target.name}"
end

def build_executable_binary( target )

    objects        = get_objects_to_link(target).join(" ")
    binary         = get_binary( target )
    defines        = target.defines.map{|d| "-D\"#{d}\"" }.join(" ")
    linker_flags   = target.linker_flags.join(" ")

    FileUtils.mkdir_p File.dirname(binary)

    sh "#{CXX_COMPILER} #{defines} -o #{binary} #{objects} #{linker_flags}", verbose: VERBOSE
end

def compile_file(src, target)

    # Generate stringified version of the flags
    # TODO: store this in a cache?
    includes     = target.includes.map{|path| "-I#{path}"}.join(" ")
    cxx_flags    = target.cxx_flags.join(" ")
    c_flags      = target.c_flags.join(" ")
    defines      = target.defines.map{|d| "-D\"#{d}\"" }.join(" ")
    linker_flags = target.linker_flags.join(" -l")
    compiler_flags = target.compiler_flags.join(" ")

    if File.extname( src ) == ".cpp"
        # TODO: add a regular flags for both, and remove this c_flags below
       compiler = "#{CXX_COMPILER} #{cxx_flags} #{compiler_flags}"
    else
       compiler = "#{C_COMPILER} #{c_flags} #{compiler_flags}"
    end

    obj = src_to_obj( src )
    dep = src_to_dep( src )
    dep_flags = "-MD -MF#{dep}"

    
    if VERBOSE
        puts "-- obj: #{obj}"
        puts "-- dep: #{dep}"
    end

    FileUtils.mkdir_p File.dirname(obj)
    FileUtils.mkdir_p File.dirname(dep)

    sh "#{compiler} -c #{includes} #{defines} #{dep_flags} -o #{obj} #{src}", verbose: VERBOSE
end

def tasks_for_target(target)

    desc "Clean #{target.name}'s intermediate files"
    task :clean do
        FileUtils.rm_f get_objects(target)
    end

    if target.type == TargetType::EXECUTABLE
        desc "Run the #{target.name}"
        task :run => [ :build ] do
            sh "./#{get_binary(target)}"
        end
    end

    desc "Clean and build target #{target.name}"
    task :rebuild => [:clean, :build]

    desc "Compile #{target.name}"
    task :build => get_binary(target) do
        if target.asset_folder_path
            copy_assets_to( BIN_DIR, target ) # easier to copy there to run the app immediately
        end
    end

    file get_binary(target) => :link do
        case target.type
        when TargetType::EXECUTABLE
            puts "#{target.name} Linking ..."
            build_executable_binary( target )
            puts "#{target.name} Linking OK"

        when TargetType::STATIC_LIBRARY
            puts "#{target.name} Creating static library ..."
            build_static_library( target )
            puts "#{target.name} Static library OK"
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
