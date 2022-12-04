require 'mkmf'
require 'timeout'

# For debugging
# CONFIG["optflags"] = "-O0"
# CONFIG["debugflags"] = "-ggdb3"

cmake_flags = [ ENV["CMAKE_FLAGS"] ]
cmake_flags << "-DLEXBOR_BUILD_TESTS_CPP=OFF"
cmake_flags << "-DLEXBOR_BUILD_SHARED=OFF"
cmake_flags << "-DLEXBOR_BUILD_STATIC=ON"

def sys(cmd)
  puts " -- #{cmd}"
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

MAKE = if Gem.win_platform?
  # On Windows, Ruby-DevKit only has 'make'.
  find_executable('make')
else
  find_executable('gmake') || find_executable('make')
end

if !MAKE
  abort "ERROR: GNU make is required to build Rugged."
end

CWD = File.expand_path(File.dirname(__FILE__))
LEXBOR_DIR = File.join(CWD, '..', '..', 'vendor', 'lexbor')
INSTALL_DIR = File.join(LEXBOR_DIR, 'dist')

if !find_executable('cmake')
  abort "ERROR: CMake is required to build Rugged."
end

if !Gem.win_platform? && !find_executable('pkg-config')
  abort "ERROR: pkg-config is required to build Rugged."
end

Dir.chdir(LEXBOR_DIR) do
  Dir.mkdir("build") if !Dir.exist?("build")

  Dir.chdir("build") do
    # On Windows, Ruby-DevKit is MSYS-based, so ensure to use MSYS Makefiles.
    generator = "-G \"MSYS Makefiles\"" if Gem.win_platform?
    run_cmake(10 * 60, ".. -DCMAKE_INSTALL_PREFIX:PATH=#{INSTALL_DIR} #{cmake_flags.join(' ')} #{generator}")
    sys("#{MAKE} install")
  end
end

# Prepend the vendored lexbor build dir to the $DEFLIBPATH.
#
# By default, $DEFLIBPATH includes $(libpath), which usually points
# to something like /usr/lib for system ruby versions (not those
# installed through rbenv or rvm).
#
# This was causing system-wide lexbor installations to be preferred
# over of our vendored lexbor version when building rugged.
#
# By putting the path to the vendored lexbor library at the front of
# $DEFLIBPATH, we can ensure that our bundled version is always used.
$DEFLIBPATH.unshift("#{INSTALL_DIR}/lib")
dir_config('lexbor', "#{INSTALL_DIR}/include", "#{INSTALL_DIR}/lib")

unless have_library 'lexbor_static' and have_header 'lexbor/html/html.h'
  abort "ERROR: Failed to build lexbor"
end

create_makefile('lexbor/lexbor')
