from conans import ConanFile, CMake, AutoToolsBuildEnvironment, tools
from conans.errors import ConanException

import json
import os


class NcursesConan(ConanFile):
    name = 'ncurses'
    version = '6.1'
    license = 'MIT'
    url = 'https://www.gnu.org/software/ncurses'
    description = 'Library of functions that manage an application\'s display on character-cell terminals'
    settings = 'os', 'compiler', 'build_type', 'arch'
    options = {'shared': [True, False],
               'fPIC': [True, False]}
    default_options = ('shared=False',
                       'fPIC=True')
    generators = ()
    exports_sources = ('CMakeLists.txt', 'version.json.in', )

    @property
    def ncurses_source_dir(self):
        return os.path.join(self.source_folder, 'source_subdirectory')

    @property
    def is_mingw(self):
        return self.settings.os == 'Windows' and self.settings.compiler == 'gcc'

    @property
    def is_msvc(self):
        return self.settings.compiler == 'Visual Studio'

    def source(self):
        source_url = 'https://invisible-mirror.net/archives/ncurses//ncurses-{version}.tar.gz'
        tools.get(source_url.format(version=self.version))
        extracted_dir = 'ncurses-{version}'.format(version=self.version)
        os.rename(extracted_dir, self.ncurses_source_dir)

    def configure(self):
        if self.settings.compiler == 'Visual Studio':
            del self.options.fPIC

    def detect_compilers(self):
        """
        Get more information about current compiler.
        :return: dict with more information
        """
        cmake = CMake(self)
        detect_dir = os.path.join(self.build_folder, 'cmake_detect')
        cmake.configure(source_dir=self.source_folder, build_dir=detect_dir)
        version = json.load(open(os.path.join(detect_dir, 'version.json')))
        return version

    def build(self):
        win_bash = False
        if self.is_mingw or self.is_msvc:
            win_bash = True

        config_args = [
            '--enable-const',
            '--disable-overwrite',
            '--without-tests',
            '--without-manpages',
            '--without-progs',
            '--enable-sp-funcs',
        ]
        compilers = self.detect_compilers()
        if compilers['CXX']['runs']:
            build_cc = compilers['CC']['path']
        else:
            build_cc = self.env['CC_HOST']
        config_args.append('--with-build-cc="{}"'.format(build_cc))

        if self.options.shared and self.settings.build_type == 'Debug':
            raise ConanException('conan-ncurses does not support both debug and shared library build')
        if self.options.shared:
            config_args.extend(['--with-shared', '--with-cxx-shared', '--without-normal'])
        else:
            config_args.extend(['--without-shared', '--without-cxx-shared'])
            if self.settings.build_type == 'Release':
                config_args.extend(['--with-normal'])
            else:
                config_args.extend(['--without-normal', '--with-debug'])
        if self.settings.build_type == 'Release':
            config_args.extend(['--without-debug'])
        extra_cflags = []
        if self.settings.compiler != 'Visual Studio':
            extra_cflags.append('-fPIC' if self.options.fPIC else '-no-pie')
        config_args.append('--with-build-cflags="{}"'.format(' '.join(extra_cflags)))

        config = AutoToolsBuildEnvironment(self, win_bash=win_bash)
        config.configure(configure_dir=self.ncurses_source_dir, args=config_args)

        config.make()
        config.install()

    def package(self):
        self.copy('*.h', dst='include', src='include')
        self.copy('*hello.lib', dst='lib', keep_path=False)
        self.copy('*.dll', dst='bin', keep_path=False)
        self.copy('*.so', dst='lib', keep_path=False)
        self.copy('*.dylib', dst='lib', keep_path=False)
        self.copy('*.a', dst='lib', keep_path=False)

    def package_info(self):
        class LibFilter:
            def __init__(self, libs):
                self.libs = set(libs)
            def get_remove(self, prefix):
                match = set(l for l in self.libs if l.startswith(prefix))
                self.libs = self.libs - match
                return list(match)
        libFilter = LibFilter(tools.collect_libs(self))

        self.cpp_info.libs = libFilter.get_remove('ncurses++') + \
                             libFilter.get_remove('menu') + \
                             libFilter.get_remove('panel') + \
                             libFilter.get_remove('form') + \
                             libFilter.get_remove('ncurses') + \
                             list(libFilter.libs)

        self.cpp_info.includedirs.append(os.path.join('include', 'ncurses'))
        self.cpp_info.defines += ['NCURSES_CONST']
