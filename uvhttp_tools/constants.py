"""Constants for UVHTTP configuration tools."""

# Default configuration values
DEFAULT_BUILD_TYPE = 'Release'
DEFAULT_ALLOCATOR = 'mimalloc'
DEFAULT_INSTALL_PREFIX = '/usr/local'
DEFAULT_TERMINAL_WIDTH = 80

# Timeout values (in seconds)
DEFAULT_TIMEOUT = 10
INPUT_TIMEOUT = 60
CMAKE_TIMEOUT = 15
MAKE_TIMEOUT = 300

# Version requirements
MIN_CMAKE_VERSION = (3, 10, 0)
MIN_PYTHON_VERSION = (3, 6, 0)

# Feature flags
FEATURE_WEBSOCKET = True
FEATURE_STATIC_FILES = True
FEATURE_RATE_LIMIT = True
FEATURE_TLS = True

# Build components
BUILD_TESTS = True
BUILD_EXAMPLES = True
BUILD_BENCHMARKS = True

# Allocator types
ALLOCATOR_SYSTEM = 'system'
ALLOCATOR_MIMALLOC = 'mimalloc'
ALLOCATOR_CUSTOM = 'custom'

# Platform names
PLATFORM_LINUX = 'Linux'
PLATFORM_MACOS = 'Darwin'
PLATFORM_WINDOWS = 'Windows'

# Architecture types
ARCH_64_BIT = '64-bit'
ARCH_32_BIT = '32-bit'
ARCH_ARM64 = 'ARM64'

# Compiler names
COMPILER_GCC = 'gcc'
COMPILER_CLANG = 'clang'
COMPILER_CC = 'cc'

# CMake flags
CMAKE_BUILD_TYPE_DEBUG = 'Debug'
CMAKE_BUILD_TYPE_RELEASE = 'Release'
CMAKE_BUILD_TYPE_RELWITHDEBINFO = 'RelWithDebInfo'

# Memory allocator types
ALLOCATOR_TYPE_SYSTEM = 0
ALLOCATOR_TYPE_MIMALLOC = 1
ALLOCATOR_TYPE_CUSTOM = 2

# Feature module flags
FEATURE_WEBSOCKET_FLAG = 'BUILD_WITH_WEBSOCKET'
FEATURE_STATIC_FILES_FLAG = 'BUILD_WITH_STATIC_FILES'
FEATURE_RATE_LIMIT_FLAG = 'BUILD_WITH_RATE_LIMIT'
FEATURE_TLS_FLAG = 'BUILD_WITH_TLS'

# Build component flags
BUILD_TESTS_FLAG = 'BUILD_TESTS'
BUILD_EXAMPLES_FLAG = 'BUILD_EXAMPLES'
BUILD_BENCHMARKS_FLAG = 'BUILD_BENCHMARKS'

# Install directories
INSTALL_BIN_DIR = 'bin'
INSTALL_LIB_DIR = 'lib'
INSTALL_INCLUDE_DIR = 'include'
INSTALL_DOC_DIR = 'share/doc/uvhttp'

# File and directory names
BUILD_DIR_NAME = 'build'
CONFIG_FILE_NAME = 'uvhttp_config.json'
LOG_FILE_NAME = 'uvhttp_build.log'

# Progress characters
PROGRESS_CHAR = '█'
PROGRESS_EMPTY_CHAR = '░'

# Symbol characters
SYMBOL_SUCCESS = '✓'
SYMBOL_ERROR = '✗'
SYMBOL_WARNING = '⚠'
SYMBOL_INFO = 'ℹ'
SYMBOL_LOADING = '⏳'
SYMBOL_ARROW = '→'
SYMBOL_BULLET = '•'

# Error messages
ERROR_MISSING_DEPENDENCY = 'Missing required dependency'
ERROR_INVALID_INPUT = 'Invalid input'
ERROR_PERMISSION_DENIED = 'Permission denied'
ERROR_TIMEOUT = 'Operation timed out'
ERROR_COMPILATION_FAILED = 'Compilation failed'
ERROR_CONFIG_FAILED = 'Configuration failed'

# Success messages
SUCCESS_BUILD_COMPLETE = 'Build completed successfully'
SUCCESS_CONFIG_COMPLETE = 'Configuration completed successfully'
SUCCESS_TESTS_PASSED = 'All tests passed'

# Warning messages
WARNING_DEPRECATED_OPTION = 'This option is deprecated'
WARNING_EXPERIMENTAL_FEATURE = 'This feature is experimental'

# Information messages
INFO_BUILD_STARTED = 'Build started'
INFO_CONFIG_STARTED = 'Configuration started'
INFO_TESTS_STARTED = 'Tests started'

# Help messages
HELP_QUICK = 'Quick build with default options'
HELP_CONFIGURE = 'Interactive configuration'
HELP_DEBUG = 'Enable debug mode'
HELP_VERBOSE = 'Enable verbose output'
HELP_CLEAN = 'Clean build directory'
HELP_TEST = 'Run tests after build'
HELP_INSTALL = 'Install to system'
HELP_PREFIX = 'Installation prefix'
HELP_JOBS = 'Number of parallel jobs'

# Environment variable names
ENV_NO_COLOR = 'NO_COLOR'
ENV_TERM = 'TERM'
ENV_DEBUG = 'DEBUG'
ENV_VERBOSE = 'VERBOSE'

# Terminal control
TERM_CLEAR = '\033[2J'
TERM_RESET = '\033[H'
TERM_HOME = '\033[H'

# Exit codes
EXIT_SUCCESS = 0
EXIT_ERROR = 1
EXIT_TIMEOUT = 2
EXIT_INTERRUPTED = 130

# Menu options
MENU_OPTION_YES = 'Yes'
MENU_OPTION_NO = 'No'
MENU_OPTION_CANCEL = 'Cancel'
MENU_OPTION_BACK = 'Back'
MENU_OPTION_NEXT = 'Next'
MENU_OPTION_FINISH = 'Finish'

# Validation patterns
VALID_INSTALL_PREFIX_PATTERN = r'^[a-zA-Z0-9_\-/\.]+$'
VALID_VERSION_PATTERN = r'^\d+\.\d+\.\d+$'

# Cache keys
CACHE_CMAKE_VERSION = 'cmake_version'
CACHE_COMPILER_VERSION = 'compiler_version'
CACHE_SYSTEM_INFO = 'system_info'

# Log levels
LOG_LEVEL_DEBUG = 'DEBUG'
LOG_LEVEL_INFO = 'INFO'
LOG_LEVEL_WARNING = 'WARNING'
LOG_LEVEL_ERROR = 'ERROR'
LOG_LEVEL_CRITICAL = 'CRITICAL'

# Color names
COLOR_RED = 'RED'
COLOR_GREEN = 'GREEN'
COLOR_YELLOW = 'YELLOW'
COLOR_BLUE = 'BLUE'
COLOR_MAGENTA = 'MAGENTA'
COLOR_CYAN = 'CYAN'
COLOR_WHITE = 'WHITE'
COLOR_RESET = 'RESET'

# Text styles
STYLE_BOLD = 'BOLD'
STYLE_DIM = 'DIM'
STYLE_UNDERLINE = 'UNDERLINE'
STYLE_BLINK = 'BLINK'
STYLE_REVERSE = 'REVERSE'

# Build steps
STEP_SYSTEM_DETECTION = 'System Detection'
STEP_BUILD_CONFIG = 'Build Configuration'
STEP_MEMORY_ALLOCATOR = 'Memory Allocator'
STEP_FEATURE_MODULES = 'Feature Modules'
STEP_BUILD_COMPONENTS = 'Build Components'
STEP_ADVANCED_OPTIONS = 'Advanced Options'
STEP_CONFIG_SUMMARY = 'Configuration Summary'
STEP_BUILD_COMMAND = 'Build Command'

# Validation errors
VALIDATION_ERROR_EMPTY = 'Input cannot be empty'
VALIDATION_ERROR_INVALID_FORMAT = 'Invalid format'
VALIDATION_ERROR_OUT_OF_RANGE = 'Value out of range'
VALIDATION_ERROR_NOT_FOUND = 'Not found'
VALIDATION_ERROR_PERMISSION = 'Permission denied'

# System checks
CHECK_COMPILER = 'Compiler'
CHECK_CMAKE = 'CMake'
CHECK_MAKE = 'Make'
CHECK_GIT = 'Git'
CHECK_PYTHON = 'Python'

# Build types
BUILD_TYPE_DEBUG = 'Debug'
BUILD_TYPE_RELEASE = 'Release'
BUILD_TYPE_RELWITHDEBINFO = 'RelWithDebInfo'
BUILD_TYPE_MINSIZEREL = 'MinSizeRel'

# Optimization levels
OPT_LEVEL_O0 = 'O0'  # No optimization
OPT_LEVEL_O1 = 'O1'  # Basic optimization
OPT_LEVEL_O2 = 'O2'  # Standard optimization
OPT_LEVEL_O3 = 'O3'  # Aggressive optimization
OPT_LEVEL_OS = 'Os'  # Optimize for size
OPT_LEVEL_OZ = 'Oz'  # Aggressive size optimization
