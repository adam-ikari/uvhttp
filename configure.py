#!/usr/bin/env python3
"""
UVHTTP Configuration Tool

A modern, standard Python-based configuration tool with minimal dependencies.
Uses only Python standard library.

Features:
- Rich terminal UI with colors and formatting
- Interactive menu system
- System detection and intelligent defaults
- Configuration validation
- CMake command generation
"""

import os
import sys
import platform
import subprocess
import shutil
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# =============================================================================
# Terminal Colors and Formatting
# =============================================================================

class Colors:
    """ANSI color codes for terminal output"""

    # Text colors
    BLACK = '\033[30m'
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    WHITE = '\033[37m'

    # Bright text colors
    BRIGHT_BLACK = '\033[90m'
    BRIGHT_RED = '\033[91m'
    BRIGHT_GREEN = '\033[92m'
    BRIGHT_YELLOW = '\033[93m'
    BRIGHT_BLUE = '\033[94m'
    BRIGHT_MAGENTA = '\033[95m'
    BRIGHT_CYAN = '\033[96m'
    BRIGHT_WHITE = '\033[97m'

    # Background colors
    BG_BLACK = '\033[40m'
    BG_RED = '\033[41m'
    BG_GREEN = '\033[42m'
    BG_YELLOW = '\033[43m'
    BG_BLUE = '\033[44m'
    BG_MAGENTA = '\033[45m'
    BG_CYAN = '\033[46m'
    BG_WHITE = '\033[47m'

    # Text styles
    BOLD = '\033[1m'
    DIM = '\033[2m'
    ITALIC = '\033[3m'
    UNDERLINE = '\033[4m'
    BLINK = '\033[5m'
    REVERSE = '\033[7m'
    HIDDEN = '\033[8m'
    STRIKETHROUGH = '\033[9m'

    # Reset
    RESET = '\033[0m'

    @staticmethod
    def disable():
        """Disable colors for non-TTY output"""
        Colors.BLACK = Colors.RED = Colors.GREEN = Colors.YELLOW = ''
        Colors.BLUE = Colors.MAGENTA = Colors.CYAN = Colors.WHITE = ''
        Colors.BRIGHT_BLACK = Colors.BRIGHT_RED = Colors.BRIGHT_GREEN = ''
        Colors.BRIGHT_YELLOW = Colors.BRIGHT_BLUE = Colors.BRIGHT_MAGENTA = ''
        Colors.BRIGHT_CYAN = Colors.BRIGHT_WHITE = ''
        Colors.BOLD = Colors.DIM = Colors.ITALIC = Colors.UNDERLINE = ''
        Colors.BLINK = Colors.REVERSE = Colors.HIDDEN = Colors.STRIKETHROUGH = ''
        Colors.RESET = ''

    @staticmethod
    def enable_if_tty():
        """Enable colors only if output is TTY"""
        if not sys.stdout.isatty():
            Colors.disable()


# =============================================================================
# Terminal UI Components
# =============================================================================

class Terminal:
    """Terminal UI helper"""

    @staticmethod
    def clear():
        """Clear the terminal screen"""
        os.system('cls' if os.name == 'nt' else 'clear')

    @staticmethod
    def get_width() -> int:
        """Get terminal width"""
        try:
            return shutil.get_terminal_size().columns
        except:
            return 80

    @staticmethod
    def print_header(title: str, width: Optional[int] = None):
        """Print a formatted header"""
        if width is None:
            width = Terminal.get_width()
        padding = 4
        line_width = width - 2 * padding

        print()
        print(f"{Colors.BOLD}{Colors.CYAN}{'═' * line_width}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}{' ' * padding}{title}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}{'═' * line_width}{Colors.RESET}")
        print()

    @staticmethod
    def print_section(title: str):
        """Print a section header"""
        print()
        print(f"{Colors.BOLD}{Colors.BLUE}{title}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.BLUE}{'─' * len(title)}{Colors.RESET}")
        print()

    @staticmethod
    def print_separator():
        """Print a separator line"""
        print()
        print(f"{Colors.DIM}{'─' * Terminal.get_width()}{Colors.RESET}")
        print()

    @staticmethod
    def print_success(message: str):
        """Print a success message"""
        print(f"{Colors.BRIGHT_GREEN}✓{Colors.RESET} {Colors.GREEN}{message}{Colors.RESET}")

    @staticmethod
    def print_error(message: str):
        """Print an error message"""
        print(f"{Colors.BRIGHT_RED}✗{Colors.RESET} {Colors.RED}{message}{Colors.RESET}", file=sys.stderr)

    @staticmethod
    def print_warning(message: str):
        """Print a warning message"""
        print(f"{Colors.BRIGHT_YELLOW}⚠{Colors.RESET} {Colors.YELLOW}{message}{Colors.RESET}")

    @staticmethod
    def print_info(message: str):
        """Print an info message"""
        print(f"{Colors.BRIGHT_CYAN}ℹ{Colors.RESET} {Colors.CYAN}{message}{Colors.RESET}")

    @staticmethod
    def print_option(key: str, value: str, description: str):
        """Print a configuration option"""
        print(f"  {Colors.BOLD}{Colors.CYAN}{key}{Colors.RESET} "
              f"[{Colors.BRIGHT_GREEN}{value}{Colors.RESET}] "
              f"- {Colors.DIM}{description}{Colors.RESET}")

    @staticmethod
    def print_help(message: str):
        """Print help text"""
        print(f"{Colors.DIM}{Colors.UNDERLINE}Help:{Colors.RESET}")
        print(f"{Colors.DIM}{message}{Colors.RESET}")
        print()


# =============================================================================
# System Detection
# =============================================================================

class SystemInfo:
    """System information detection"""

    def __init__(self):
        self.os_name = platform.system()
        self.os_version = platform.version()
        self.arch = platform.machine()
        self.processor = platform.processor()
        self.python_version = platform.python_version()
        self.cpu_count = os.cpu_count() or 1

        # Detect compiler
        self.compiler = self._detect_compiler()
        self.compiler_version = self._get_compiler_version()

        # Detect CMake
        self.cmake_version = self._get_cmake_version()

        # Determine architecture type
        self.arch_type = self._determine_arch_type()

    def _detect_compiler(self) -> str:
        """Detect available C compiler"""
        for compiler in ['gcc', 'clang', 'cc']:
            if shutil.which(compiler):
                return compiler
        return 'none'

    def _get_compiler_version(self) -> str:
        """Get compiler version"""
        if self.compiler == 'none':
            return 'Not found'

        try:
            result = subprocess.run(
                [self.compiler, '--version'],
                capture_output=True,
                text=True,
                timeout=5
            )
            return result.stdout.split('\n')[0]
        except:
            return 'Unknown'

    def _get_cmake_version(self) -> str:
        """Get CMake version"""
        if not shutil.which('cmake'):
            return 'Not found'

        try:
            result = subprocess.run(
                ['cmake', '--version'],
                capture_output=True,
                text=True,
                timeout=5
            )
            return result.stdout.split('\n')[0]
        except:
            return 'Unknown'

    def _determine_arch_type(self) -> str:
        """Determine architecture type"""
        if self.arch in ['x86_64', 'AMD64']:
            return '64-bit'
        elif self.arch in ['i686', 'i386', 'x86']:
            return '32-bit'
        elif self.arch in ['aarch64', 'arm64', 'ARM64']:
            return 'ARM64'
        else:
            return f'unknown ({self.arch})'

    def get_cmake_flags(self) -> str:
        """Get CMake flags for this architecture"""
        if self.arch_type == '32-bit':
            return '-DCMAKE_C_FLAGS=-m32'
        return ''

    def is_compatible(self) -> bool:
        """Check if system is compatible"""
        return (
            self.compiler != 'none' and
            self.cmake_version != 'Not found' and
            self.arch_type != 'unknown'
        )


# =============================================================================
# Configuration Model
# =============================================================================

class Configuration:
    """Build configuration"""

    def __init__(self):
        self.build_type = 'Release'
        self.allocator = 'mimalloc'
        self.websocket = True
        self.static_files = True
        self.rate_limit = True
        self.tls = True
        self.build_tests = True
        self.build_examples = True
        self.build_benchmarks = True
        self.logging = True
        self.optimization = '2'
        self.install_prefix = '/usr/local'

    def to_dict(self) -> Dict[str, str]:
        """Convert configuration to dictionary"""
        return {
            'BUILD_TYPE': self.build_type,
            'ALLOCATOR': self.allocator,
            'WEBSOCKET': 'ON' if self.websocket else 'OFF',
            'STATIC_FILES': 'ON' if self.static_files else 'OFF',
            'RATE_LIMIT': 'ON' if self.rate_limit else 'OFF',
            'TLS': 'ON' if self.tls else 'OFF',
            'BUILD_TESTS': 'ON' if self.build_tests else 'OFF',
            'BUILD_EXAMPLES': 'ON' if self.build_examples else 'OFF',
            'BUILD_BENCHMARKS': 'ON' if self.build_benchmarks else 'OFF',
            'LOGGING': 'ON' if self.logging else 'OFF',
            'INSTALL_PREFIX': self.install_prefix,
        }

    def generate_cmake_command(self, system_info: SystemInfo) -> str:
        """Generate CMake build command"""
        cmd = 'mkdir -p build && cd build && cmake'

        # Build type
        cmd += f' -DCMAKE_BUILD_TYPE={self.build_type}'

        # Allocator
        if self.allocator == 'mimalloc':
            cmd += ' -DUVHTTP_ALLOCATOR_TYPE=1 -DBUILD_WITH_MIMALLOC=ON'
        elif self.allocator == 'system':
            cmd += ' -DUVHTTP_ALLOCATOR_TYPE=0 -DBUILD_WITH_MIMALLOC=OFF'
        else:
            cmd += ' -DUVHTTP_ALLOCATOR_TYPE=2'

        # Features
        cmd += f' -DBUILD_WITH_WEBSOCKET={"ON" if self.websocket else "OFF"}'
        cmd += f' -DBUILD_WITH_STATIC_FILES={"ON" if self.static_files else "OFF"}'
        cmd += f' -DBUILD_WITH_RATE_LIMIT={"ON" if self.rate_limit else "OFF"}'
        cmd += f' -DBUILD_WITH_TLS={"ON" if self.tls else "OFF"}'

        # Components
        cmd += f' -DBUILD_TESTS={"ON" if self.build_tests else "OFF"}'
        cmd += f' -DBUILD_EXAMPLES={"ON" if self.build_examples else "OFF"}'
        cmd += f' -DBUILD_BENCHMARKS={"ON" if self.build_benchmarks else "OFF"}'

        # Logging
        if not self.logging:
            cmd += ' -DUVHTTP_FEATURE_LOGGING=OFF'

        # Install prefix
        cmd += f' -DCMAKE_INSTALL_PREFIX={self.install_prefix}'

        # Architecture-specific flags
        if system_info.arch_type == '32-bit':
            cmd += ' -DCMAKE_C_FLAGS=-m32'

        # Build command
        cmd += ' .. && make -j{}'.format(system_info.cpu_count)

        return cmd


# =============================================================================
# Interactive Menu
# =============================================================================

class Menu:
    """Interactive menu system"""

    @staticmethod
    def select_option(prompt: str, options: List[str], default: int = 0) -> int:
        """Display menu and get user selection"""
        print()
        Terminal.print_question(prompt)
        print()

        for i, option in enumerate(options):
            num = i + 1
            if i == default:
                print(f"  {Colors.BOLD}{Colors.BRIGHT_GREEN}{num}){Colors.RESET} {option} {Colors.DIM}(default){Colors.RESET}")
            else:
                print(f"  {Colors.DIM}{num}){Colors.RESET} {option}")
        print()

        while True:
            try:
                choice = input(f"{Colors.BOLD}{Colors.MAGENTA}Select option [1-{len(options)}]:{Colors.RESET} ")
                if not choice:
                    return default

                choice = int(choice)
                if 1 <= choice <= len(options):
                    return choice - 1
                else:
                    Terminal.print_error(f"Please enter a number between 1 and {len(options)}")
            except ValueError:
                Terminal.print_error("Please enter a valid number")

    @staticmethod
    def ask_yes_no(prompt: str, default: bool = True) -> bool:
        """Ask a yes/no question"""
        default_str = 'Y/n' if default else 'y/N'

        while True:
            response = input(f"{Colors.BOLD}{Colors.MAGENTA}{prompt}{Colors.RESET} {Colors.DIM}[{default_str}]{Colors.RESET}: ")
            if not response:
                return default

            response = response.lower()
            if response in ['y', 'yes']:
                return True
            elif response in ['n', 'no']:
                return False
            else:
                Terminal.print_error("Please answer yes or no")

    @staticmethod
    def ask_string(prompt: str, default: str = '') -> str:
        """Ask for string input"""
        result = input(f"{Colors.BOLD}{Colors.MAGENTA}{prompt}{Colors.RESET} [{Colors.BRIGHT_GREEN}{default}{Colors.RESET}]: ")
        return result if result else default


# =============================================================================
# Configuration Wizard
# =============================================================================

class ConfigurationWizard:
    """Interactive configuration wizard"""

    def __init__(self, system_info: SystemInfo):
        self.system_info = system_info
        self.config = Configuration()
        self.steps = [
            self._step_system_info,
            self._step_build_type,
            self._step_allocator,
            self._step_features,
            self._step_components,
            self._step_advanced,
            self._step_summary,
        ]
        self.current_step = 0

    def run(self) -> bool:
        """Run the configuration wizard"""
        Terminal.clear()
        Terminal.print_header("UVHTTP Configuration Tool")

        # Check system compatibility
        if not self.system_info.is_compatible():
            Terminal.print_error("System is not compatible with UVHTTP")
            print()
            Terminal.print_info("Required:")
            if self.system_info.compiler == 'none':
                Terminal.print_option("C Compiler", "Missing", "gcc or clang")
            if self.system_info.cmake_version == 'Not found':
                Terminal.print_option("CMake", "Missing", "version 3.10+")
            print()
            Terminal.print_info("Install instructions:")
            print("  Ubuntu/Debian: sudo apt-get install build-essential cmake")
            print("  Fedora/RHEL: sudo dnf install gcc cmake make")
            print("  macOS: brew install cmake")
            return False

        # Run configuration steps
        try:
            for step in self.steps:
                self.current_step += 1
                progress = int(self.current_step * 100 / len(self.steps))
                print(f"\r{Colors.BOLD}{Colors.CYAN}Progress:{Colors.RESET} "
                      f"[{Colors.BRIGHT_GREEN}{'█' * (progress // 5)}{Colors.DIM}{'░' * (20 - progress // 5)}{Colors.RESET}] "
                      f"{progress}% ({self.current_step}/{len(self.steps)})", end='', flush=True)
                step()
                print()  # New line after progress

            return True
        except KeyboardInterrupt:
            print()
            Terminal.print_warning("Configuration cancelled")
            return False
        except Exception as e:
            print()
            Terminal.print_error(f"Configuration failed: {e}")
            return False

    def _step_system_info(self):
        """Display system information"""
        Terminal.print_section("System Information")

        Terminal.print_option("OS", self.system_info.os_name, "Operating system")
        Terminal.print_option("Architecture", self.system_info.arch_type, "System architecture")
        Terminal.print_option("CPU Cores", str(self.system_info.cpu_count), "Available cores")
        Terminal.print_option("Compiler", self.system_info.compiler, f"{self.system_info.compiler_version}")
        Terminal.print_option("CMake", self.system_info.cmake_version, "Build system")

        # Set intelligent defaults
        if self.system_info.arch_type == '32-bit':
            self.config.allocator = 'system'
            Terminal.print_info("Adjusted defaults for 32-bit architecture")

    def _step_build_type(self):
        """Configure build type"""
        Terminal.print_section("Build Configuration")

        Terminal.print_help("Build types:")
        Terminal.print_help("  Debug         - Full debugging symbols, no optimization")
        Terminal.print_help("  Release       - Optimized for performance (recommended)")
        Terminal.print_help("  RelWithDebInfo - Optimized with debug symbols")
        Terminal.print_help("  MinSizeRel    - Optimized for size")

        options = ['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel']
        choice = Menu.select_option("Select build type", options, default=1)
        self.config.build_type = options[choice]

    def _step_allocator(self):
        """Configure memory allocator"""
        Terminal.print_section("Memory Allocator")

        Terminal.print_help("Memory allocators:")
        Terminal.print_help("  mimalloc - High-performance allocator (recommended)")
        Terminal.print_help("  system   - Standard malloc/free (most compatible)")
        Terminal.print_help("  custom   - Your own allocator implementation")

        if self.system_info.arch_type == '32-bit':
            self.config.allocator = 'system'
            Terminal.print_info("Using system allocator for 32-bit builds")
        else:
            options = ['mimalloc', 'system', 'custom']
            choice = Menu.select_option("Select memory allocator", options, default=0)
            self.config.allocator = options[choice]

    def _step_features(self):
        """Configure feature modules"""
        Terminal.print_section("Feature Modules")

        Terminal.print_help("Enable or disable optional features:")

        self.config.websocket = Menu.ask_yes_no("Enable WebSocket support?", True)
        self.config.static_files = Menu.ask_yes_no("Enable static file serving?", True)
        self.config.rate_limit = Menu.ask_yes_no("Enable rate limiting?", True)
        self.config.tls = Menu.ask_yes_no("Enable TLS/SSL support?", True)

    def _step_components(self):
        """Configure build components"""
        Terminal.print_section("Build Components")

        Terminal.print_help("Select which components to build:")

        self.config.build_tests = Menu.ask_yes_no("Build unit tests?", True)
        self.config.build_examples = Menu.ask_yes_no("Build example programs?", True)
        self.config.build_benchmarks = Menu.ask_yes_no("Build benchmark programs?", True)

    def _step_advanced(self):
        """Configure advanced options"""
        Terminal.print_section("Advanced Options")

        Terminal.print_help("Configure advanced build options:")

        self.config.logging = Menu.ask_yes_no("Enable logging system?", True)
        self.config.install_prefix = Menu.ask_string("Installation prefix", self.config.install_prefix)

    def _step_summary(self):
        """Display configuration summary"""
        Terminal.print_section("Configuration Summary")

        print()
        print(f"{Colors.BOLD}{Colors.CYAN}Build Configuration:{Colors.RESET}")
        print()
        Terminal.print_option("Build Type", self.config.build_type, "Optimization level")
        Terminal.print_option("Allocator", self.config.allocator, "Memory allocation strategy")
        Terminal.print_option("Architecture", self.system_info.arch_type, "Target architecture")
        Terminal.print_option("Install Prefix", self.config.install_prefix, "Installation directory")

        print()
        print(f"{Colors.BOLD}{Colors.CYAN}Feature Modules:{Colors.RESET}")
        print()
        Terminal.print_option("WebSocket", "ON" if self.config.websocket else "OFF", "WebSocket support")
        Terminal.print_option("Static Files", "ON" if self.config.static_files else "OFF", "File serving")
        Terminal.print_option("Rate Limiting", "ON" if self.config.rate_limit else "OFF", "Request limiting")
        Terminal.print_option("TLS/SSL", "ON" if self.config.tls else "OFF", "Encrypted connections")

        print()
        print(f"{Colors.BOLD}{Colors.CYAN}Build Components:{Colors.RESET}")
        print()
        Terminal.print_option("Tests", "ON" if self.config.build_tests else "OFF", "Unit tests")
        Terminal.print_option("Examples", "ON" if self.config.build_examples else "OFF", "Example programs")
        Terminal.print_option("Benchmarks", "ON" if self.config.build_benchmarks else "OFF", "Performance tests")
        Terminal.print_option("Logging", "ON" if self.config.logging else "OFF", "Debug logging")

        print()

        # Confirm configuration
        Terminal.print_separator()
        if not Menu.ask_yes_no("Use this configuration?", True):
            raise KeyboardInterrupt("Configuration cancelled")


# =============================================================================
# Main Application
# =============================================================================

def main():
    """Main application entry point"""

    # Enable colors only for TTY
    Colors.enable_if_tty()

    # Detect system information
    system_info = SystemInfo()

    # Run configuration wizard
    wizard = ConfigurationWizard(system_info)
    if not wizard.run():
        return 1

    # Generate build command
    Terminal.print_section("Build Command")

    build_cmd = wizard.config.generate_cmake_command(system_info)

    print()
    print(f"{Colors.BOLD}{Colors.CYAN}To build UVHTTP, run:{Colors.RESET}")
    print()
    print(f"{Colors.BRIGHT_GREEN}{build_cmd}{Colors.RESET}")
    print()

    # Option to run build
    Terminal.print_separator()
    if Menu.ask_yes_no("Run build now?", False):
        Terminal.print_info("Starting build...")
        print()

        try:
            result = subprocess.run(
                build_cmd,
                shell=True,
                check=True
            )

            Terminal.print_separator()
            Terminal.print_header("Build Successful!")

            print()
            Terminal.print_success("UVHTTP has been built successfully!")
            print()
            print(f"{Colors.BOLD}{Colors.CYAN}Quick Start:{Colors.RESET}")
            print()
            print(f"  {Colors.DIM}cd build{Colors.RESET}")
            print(f"  {Colors.DIM}./dist/bin/hello_world{Colors.RESET}")
            print()

            return 0

        except subprocess.CalledProcessError as e:
            Terminal.print_error("Build failed")
            return 1
    else:
        Terminal.print_info("Build command generated. Run it manually later.")
        print()
        Terminal.print_info("Save the command for later use:")
        print()
        print(f"{Colors.DIM}{build_cmd}{Colors.RESET}")
        print()

    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print()
        Terminal.print_warning("Operation cancelled")
        sys.exit(1)
    except Exception as e:
        Terminal.print_error(f"Unexpected error: {e}")
        sys.exit(1)
