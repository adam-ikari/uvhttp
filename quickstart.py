#!/usr/bin/env python3
"""
UVHTTP Quick Start Tool

A modern Python-based quick start tool with minimal dependencies.
Uses only Python standard library.

Features:
- Automatic system detection and configuration
- Parallel compilation with optimal settings
- Progress tracking and visual feedback
- Comprehensive error handling and recovery
- Post-build validation and testing
"""

import os
import sys
import platform
import subprocess
import shutil
import argparse
from pathlib import Path
from typing import Optional, Tuple


# =============================================================================
# Terminal Colors and Formatting
# =============================================================================

class Colors:
    """ANSI color codes for terminal output"""

    # Basic colors
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    WHITE = '\033[37m'

    # Bright colors
    BRIGHT_RED = '\033[91m'
    BRIGHT_GREEN = '\033[92m'
    BRIGHT_YELLOW = '\033[93m'
    BRIGHT_BLUE = '\033[94m'
    BRIGHT_MAGENTA = '\033[95m'
    BRIGHT_CYAN = '\033[96m'
    BRIGHT_WHITE = '\033[97m'

    # Styles
    BOLD = '\033[1m'
    DIM = '\033[2m'
    UNDERLINE = '\033[4m'
    RESET = '\033[0m'

    @staticmethod
    def disable():
        """Disable colors for non-TTY output"""
        for attr in dir(Colors):
            if not attr.startswith('_') and isinstance(getattr(Colors, attr), str):
                setattr(Colors, attr, '')

    @staticmethod
    def enable_if_tty():
        """Enable colors only if output is TTY"""
        if not sys.stdout.isatty():
            Colors.disable()


# =============================================================================
# Terminal UI
# =============================================================================

class Terminal:
    """Terminal UI helper"""

    @staticmethod
    def clear():
        """Clear terminal screen"""
        os.system('cls' if os.name == 'nt' else 'clear')

    @staticmethod
    def print_header(title: str):
        """Print formatted header"""
        print()
        print(f"{Colors.BOLD}{Colors.CYAN}{'═' * 60}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}  {title}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}{'═' * 60}{Colors.RESET}")
        print()

    @staticmethod
    def print_section(title: str):
        """Print section header"""
        print()
        print(f"{Colors.BOLD}{Colors.BLUE}{title}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.BLUE}{'─' * len(title)}{Colors.RESET}")
        print()

    @staticmethod
    def print_separator():
        """Print separator line"""
        print()
        print(f"{Colors.DIM}{'─' * 60}{Colors.RESET}")
        print()

    @staticmethod
    def print_success(message: str):
        """Print success message"""
        print(f"{Colors.BRIGHT_GREEN}✓{Colors.RESET} {Colors.GREEN}{message}{Colors.RESET}")

    @staticmethod
    def print_error(message: str):
        """Print error message"""
        print(f"{Colors.BRIGHT_RED}✗{Colors.RESET} {Colors.RED}{message}{Colors.RESET}", file=sys.stderr)

    @staticmethod
    def print_warning(message: str):
        """Print warning message"""
        print(f"{Colors.BRIGHT_YELLOW}⚠{Colors.RESET} {Colors.YELLOW}{message}{Colors.RESET}")

    @staticmethod
    def print_info(message: str):
        """Print info message"""
        print(f"{Colors.BRIGHT_CYAN}ℹ{Colors.RESET} {Colors.CYAN}{message}{Colors.RESET}")

    @staticmethod
    def show_progress(message: str):
        """Show progress indicator"""
        print(f"{Colors.BRIGHT_CYAN}⏳{Colors.RESET} {Colors.DIM}{message}...{Colors.RESET}", end='', flush=True)

    @staticmethod
    def complete_progress(message: str):
        """Complete progress indicator"""
        print(f"\r{Colors.BRIGHT_GREEN}✓{Colors.RESET} {Colors.GREEN}{message}{Colors.RESET}")


# =============================================================================
# System Detection
# =============================================================================

class SystemInfo:
    """System information detection"""

    def __init__(self):
        self.os_name = platform.system()
        self.arch = platform.machine()
        self.cpu_count = os.cpu_count() or 1

        # Detect compiler
        self.compiler = self._detect_compiler()
        self.cmake_version = self._get_cmake_version()

        # Determine architecture type
        self.arch_type = self._determine_arch_type()

    def _detect_compiler(self) -> str:
        """Detect C compiler"""
        for compiler in ['gcc', 'clang', 'cc']:
            if shutil.which(compiler):
                return compiler
        return 'none'

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
        else:
            return f'unknown ({self.arch})'

    def get_cmake_flags(self) -> str:
        """Get CMake flags"""
        if self.arch_type == '32-bit':
            return '-DCMAKE_C_FLAGS=-m32'
        return ''

    def is_compatible(self) -> bool:
        """Check compatibility"""
        return (
            self.compiler != 'none' and
            self.cmake_version != 'Not found'
        )


# =============================================================================
# Quick Start Builder
# =============================================================================

class QuickStartBuilder:
    """Quick start build manager"""

    def __init__(self, args):
        self.args = args
        self.system_info = SystemInfo()
        self.build_dir = Path('build')

    def run(self) -> int:
        """Run quick start build"""
        Terminal.clear()
        Terminal.print_header("UVHTTP Quick Start")

        # Check compatibility
        if not self._check_compatibility():
            return 1

        # Run build steps
        try:
            self._setup_build_directory()
            self._configure_cmake()
            self._compile_project()
            if self.args.test:
                self._run_tests()
            self._validate_build()

            # Print summary
            self._print_summary()

            return 0

        except KeyboardInterrupt:
            print()
            Terminal.print_warning("Build cancelled")
            return 1
        except Exception as e:
            Terminal.print_error(f"Build failed: {e}")
            return 1

    def _check_compatibility(self) -> bool:
        """Check system compatibility"""
        Terminal.print_info("Checking dependencies...")

        missing = []

        if self.system_info.compiler == 'none':
            missing.append("C compiler (gcc or clang)")

        if self.system_info.cmake_version == 'Not found':
            missing.append("cmake")

        if not shutil.which('make'):
            missing.append("make")

        if not shutil.which('git'):
            missing.append("git")

        if missing:
            Terminal.print_error("Missing dependencies:")
            for dep in missing:
                print(f"  {Colors.RED}✗{Colors.RESET} {dep}")
            print()
            Terminal.print_error("Install instructions:")
            print("  Ubuntu/Debian: sudo apt-get install build-essential cmake git")
            print("  Fedora/RHEL: sudo dnf install gcc cmake make git")
            print("  macOS: brew install cmake git")
            return False

        Terminal.complete_progress("Dependencies OK")
        return True

    def _setup_build_directory(self):
        """Setup build directory"""
        Terminal.show_progress("Setting up build directory")

        if self.build_dir.exists():
            if self.args.clean:
                shutil.rmtree(self.build_dir)

        self.build_dir.mkdir(parents=True, exist_ok=True)

        Terminal.complete_progress("Build directory ready")

    def _configure_cmake(self):
        """Configure with CMake"""
        Terminal.show_progress("Configuring with CMake")

        cmake_cmd = ['cmake', '..']
        cmake_cmd.append(f'-DCMAKE_BUILD_TYPE={self.args.build_type}')

        # Add architecture flags
        cmake_flags = self.system_info.get_cmake_flags()
        if cmake_flags:
            cmake_cmd.append(cmake_flags)

        # Add mimalloc for 64-bit
        if self.system_info.arch_type == '64-bit':
            cmake_cmd.append('-DBUILD_WITH_MIMALLOC=ON')

        # Add verbose if requested
        if self.args.verbose:
            cmake_cmd.append('-DCMAKE_VERBOSE_MAKEFILE=ON')

        if self.args.verbose:
            print(f"\n{Colors.DIM}Running: {' '.join(cmake_cmd)}{Colors.RESET}")

        subprocess.run(
            cmake_cmd,
            cwd=self.build_dir,
            check=True
        )

        Terminal.complete_progress("CMake configuration successful")

    def _compile_project(self):
        """Compile project"""
        Terminal.show_progress(f"Compiling with {self.system_info.cpu_count} cores")

        make_cmd = ['make', f'-j{self.system_info.cpu_count}']

        if self.args.verbose:
            print(f"\n{Colors.DIM}Running: {' '.join(make_cmd)}{Colors.RESET}")

        subprocess.run(
            make_cmd,
            cwd=self.build_dir,
            check=True
        )

        Terminal.complete_progress("Compilation successful")

    def _run_tests(self):
        """Run tests"""
        Terminal.show_progress("Running tests")

        try:
            subprocess.run(
                ['make', 'test'],
                cwd=self.build_dir,
                check=True
            )
            Terminal.complete_progress("All tests passed")
        except subprocess.CalledProcessError:
            Terminal.print_warning("Some tests failed, but build was successful")

    def _validate_build(self):
        """Validate build"""
        Terminal.show_progress("Validating build")

        required_files = [
            self.build_dir / 'dist' / 'lib' / 'libuvhttp.a',
            self.build_dir / 'dist' / 'bin' / 'hello_world',
        ]

        for file in required_files:
            if not file.exists():
                raise RuntimeError(f"Missing required file: {file}")

        Terminal.complete_progress("Build validation successful")

    def _print_summary(self):
        """Print build summary"""
        Terminal.print_section("Build Summary")

        print(f"  OS:           {Colors.CYAN}{self.system_info.os_name}{Colors.RESET}")
        print(f"  Architecture: {Colors.CYAN}{self.system_info.arch_type}{Colors.RESET}")
        print(f"  CPU Cores:    {Colors.CYAN}{self.system_info.cpu_count}{Colors.RESET}")
        print(f"  Compiler:     {Colors.CYAN}{self.system_info.compiler}{Colors.RESET}")
        print()

        print(f"  Build Type:   {Colors.CYAN}{self.args.build_type}{Colors.RESET}")
        print(f"  Directory:    {Colors.CYAN}{self.build_dir.absolute()}{Colors.RESET}")
        print(f"  Tests Run:    {Colors.CYAN}{'Yes' if self.args.test else 'No'}{Colors.RESET}")
        print()

        print(f"{Colors.BOLD}{Colors.CYAN}Quick Start:{Colors.RESET}")
        print(f"  {Colors.DIM}Run hello world:{Colors.RESET}")
        print(f"    {Colors.GREEN}./dist/bin/hello_world{Colors.RESET}")
        print()
        print(f"  {Colors.DIM}Build examples:{Colors.RESET}")
        print(f"    {Colors.GREEN}cd ../examples && make -f Makefile.examples{Colors.RESET}")
        print()

        Terminal.print_header("Build Complete!")
        Terminal.print_success("UVHTTP has been built successfully!")


# =============================================================================
# Main Entry Point
# =============================================================================

def main():
    """Main entry point"""

    # Enable colors for TTY
    Colors.enable_if_tty()

    # Parse arguments
    parser = argparse.ArgumentParser(
        description='UVHTTP Quick Start Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                 # Quick build with defaults
  %(prog)s --test          # Build and run tests
  %(prog)s --debug         # Debug build
  %(prog)s --test --debug  # Debug build with tests
        """
    )

    parser.add_argument(
        '--test', '-t',
        action='store_true',
        help='Run tests after build'
    )

    parser.add_argument(
        '--debug', '-d',
        action='store_true',
        help='Build with debug symbols'
    )

    parser.add_argument(
        '--no-clean', '-n',
        action='store_true',
        help="Don't clean existing build directory"
    )

    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose output'
    )

    args = parser.parse_args()

    # Set build type
    if args.debug:
        args.build_type = 'Debug'
    else:
        args.build_type = 'Release'

    # Set clean flag
    args.clean = not args.no_clean

    # Run builder
    builder = QuickStartBuilder(args)
    return builder.run()


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
