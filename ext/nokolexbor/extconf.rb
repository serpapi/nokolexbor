require 'mkmf'
require 'timeout'

# For debugging
# CONFIG["optflags"] = "-O0"
# CONFIG["debugflags"] = "-ggdb3"

cmake_flags = [ ENV["CMAKE_FLAGS"] ]
cmake_flags << "-DLEXBOR_BUILD_TESTS_CPP=OFF"
cmake_flags << "-DLEXBOR_BUILD_SHARED=OFF"
cmake_flags << "-DLEXBOR_BUILD_STATIC=ON"
# For debugging
# cmake_flags << "-DLEXBOR_OPTIMIZATION_LEVEL='-O0 -g'"

append_cflags("-DLEXBOR_STATIC")
append_cflags("-DLIBXML_STATIC")

def sys(cmd)
  puts "-- #{cmd}"
  unless ret = xsystem(cmd)
    raise "ERROR: '#{cmd}' failed"
  end
  ret
end

# Thrown when we detect CMake is taking too long and we killed it
class CMakeTimeout < StandardError
end

def self.run_cmake(timeout, args)
  # Set to process group so we can kill it and its children
  pgroup = Gem.win_platform? ? :new_pgroup : :pgroup
  pid = Process.spawn("cmake #{args}", pgroup => true)

  Timeout.timeout(timeout) do
    Process.waitpid(pid)
  end

rescue Timeout::Error
  # Kill it, #detach is essentially a background wait, since we don't actually
  # care about waiting for it now
  Process.kill(-9, pid)
  Process.detach(pid)
  raise CMakeTimeout.new("cmake has exceeded its timeout of #{timeout}s")
end

# From: https://stackoverflow.com/questions/2108727
# Cross-platform way of finding an executable in the $PATH.
#
#   which('ruby') #=> /usr/bin/ruby
def which(cmd)
  exts = ENV['PATHEXT'] ? ENV['PATHEXT'].split(';') : ['']
  ENV['PATH'].split(File::PATH_SEPARATOR).each do |path|
    exts.each { |ext|
      exe = File.join(path, "#{cmd}#{ext}")
      return exe if File.executable? exe
    }
  end
  return nil
end

# From: https://github.com/flavorjones/mini_portile/blob/main/lib/mini_portile2/mini_portile.rb#L94
def apply_patch(patch_file, chdir)
  case
  when which('git')
    # By --work-tree=. git-apply uses the current directory as
    # the project root and will not search upwards for .git.
    Process.waitpid(Process.spawn("git --git-dir=. --work-tree=. apply #{patch_file}", chdir: chdir))
  when which('patch')
    Process.waitpid(Process.spawn("patch -p1 -i #{patch_file}", chdir: chdir))
  else
    raise "Failed to complete patch task; patch(1) or git(1) is required."
  end
end


MAKE = if Gem.win_platform?
  # On Windows, Ruby-DevKit only has 'make'.
  find_executable('make')
else
  find_executable('gmake') || find_executable('make')
end

if !MAKE
  abort "ERROR: GNU make is required to build Lexbor."
end

CWD = File.expand_path(File.dirname(__FILE__))
LEXBOR_DIR = File.join(CWD, '..', '..', 'vendor', 'lexbor')
EXT_DIR = File.join(CWD, '..', '..', 'ext', 'nokolexbor')
INSTALL_DIR = File.join(LEXBOR_DIR, 'dist')

if !find_executable('cmake')
  abort "ERROR: CMake is required to build Lexbor."
end

Dir.chdir(LEXBOR_DIR) do
  # Patch lexbor
  Dir['../../patches/*-lexbor-*.patch'].each do |patch_file|
    apply_patch(patch_file, '.')
  end

  Dir.mkdir("build") if !Dir.exist?("build")

  Dir.chdir("build") do
    # On Windows, Ruby-DevKit is MSYS-based, so ensure to use MSYS Makefiles.
    generator = "-G \"MSYS Makefiles\"" if Gem.win_platform?
    run_cmake(10 * 60, ".. -DCMAKE_INSTALL_PREFIX:PATH=#{INSTALL_DIR} #{cmake_flags.join(' ')} #{generator}")
    sys("#{MAKE} install")
  end
end

# Generate config.h and xmlversion.h for libxml2
Dir.chdir(EXT_DIR) do
  Dir.mkdir("build") if !Dir.exist?("build")

  Dir.chdir("build") do
    run_cmake(10 * 60, "..")
  end
end

# Prepend the vendored lexbor build dir to the $DEFLIBPATH.
#
# By default, $DEFLIBPATH includes $(libpath), which usually points
# to something like /usr/lib for system ruby versions (not those
# installed through rbenv or rvm).
#
# This was causing system-wide lexbor installations to be preferred
# over of our vendored lexbor version when building lexbor.
#
# By putting the path to the vendored lexbor library at the front of
# $DEFLIBPATH, we can ensure that our bundled version is always used.
$DEFLIBPATH.unshift("#{INSTALL_DIR}/lib")
dir_config('lexbor', "#{INSTALL_DIR}/include", "#{INSTALL_DIR}/lib")

unless have_library 'lexbor_static' and have_header 'lexbor/html/html.h'
  abort "ERROR: Failed to build lexbor"
end

create_makefile('nokolexbor/nokolexbor')
