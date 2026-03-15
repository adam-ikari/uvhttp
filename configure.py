#!/usr/bin/env python3
"""
UVHTTP Configuration Tool

This is an interactive configuration wizard for building UVHTTP with various options.
It provides a step-by-step interface for configuring build settings, features, and components.
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from uvhttp_tools.constants import (
    DEFAULT_BUILD_TYPE, DEFAULT_ALLOCATOR, DEFAULT_INSTALL_PREFIX,
    DEFAULT_TIMEOUT, MIN_CMAKE_VERSION, MIN_PYTHON_VERSION,
    ALLOCATOR_SYSTEM, ALLOCATOR_MIMALLOC, ALLOCATOR_CUSTOM,
    PLATFORM_LINUX, PLATFORM_MACOS, PLATFORM_WINDOWS,
    ARCH_64_BIT, ARCH_32_BIT, ARCH_ARM64,
    COMPILER_GCC, COMPILER_CLANG, COMPILER_CC,
    CMAKE_BUILD_TYPE_DEBUG, CMAKE_BUILD_TYPE_RELEASE,
    ALLOCATOR_TYPE_SYSTEM, ALLOCATOR_TYPE_MIMALLOC, ALLOCATOR_TYPE_CUSTOM,
    FEATURE_WEBSOCKET_FLAG, FEATURE_STATIC_FILES_FLAG,
    FEATURE_RATE_LIMIT_FLAG, FEATURE_TLS_FLAG,
    BUILD_TESTS_FLAG, BUILD_EXAMPLES_FLAG, BUILD_BENCHMARKS_FLAG,
    BUILD_DIR_NAME, SUCCESS_CONFIG_COMPLETE, EXIT_SUCCESS, EXIT_ERROR,
    STEP_SYSTEM_DETECTION, STEP_BUILD_CONFIG, STEP_MEMORY_ALLOCATOR,
    STEP_FEATURE_MODULES, STEP_BUILD_COMPONENTS, STEP_ADVANCED_OPTIONS,
    STEP_CONFIG_SUMMARY, STEP_BUILD_COMMAND,
    VALIDATION_ERROR_EMPTY, ERROR_CONFIG_FAILED
)

from uvhttp_tools.terminal import (
    Colors, Terminal,
    SYMBOL_SUCCESS, SYMBOL_ERROR, SYMBOL_WARNING, SYMBOL_INFO, SYMBOL_LOADING
)

from uvhttp_tools.validators import (
    ValidationError, Validators
)


class SystemInfo:
    """System information detection.
    
    Detects OS, architecture, compiler, and other system capabilities.
    """
    
    def __init__(self) -> None:
        """Initialize system information detection."""
        self.os_name: str = platform.system()
        self.arch: str = platform.machine()
        self.cpu_count: int = os.cpu_count() or 1
        self.compiler: str = self._detect_compiler()
        self.cmake_version: str = self._get_cmake_version()
        self.arch_type: str = self._determine_arch_type()
        self.python_version: Tuple[int, int, int] = sys.version_info[:3]
    
    def _detect_compiler(self) -> str:
        """Detect C compiler.
        
        Returns:
            Detected compiler name.
        """
        compilers = [COMPILER_GCC, COMPILER_CLANG, COMPILER_CC]
        for compiler in compilers:
            if shutil.which(compiler):
                return compiler
        return 'none'
    
    def _get_cmake_version(self) -> str:
        """Get CMake version.
        
        Returns:
            CMake version string or 'Not found'.
        """
        if not shutil.which('cmake'):
            return 'Not found'
        
        try:
            result = subprocess.run(
                ['cmake', '--version'],
                capture_output=True,
                text=True,
                timeout=15
            )
            if result.returncode == 0:
                return result.stdout.split('\n')[0]
            return 'Unknown'
        except subprocess.TimeoutExpired:
            return 'Timeout'
        except (subprocess.SubprocessError, FileNotFoundError):
            return 'Not found'
    
    def _determine_arch_type(self) -> str:
        """Determine architecture type.
        
        Returns:
            Architecture type string.
        """
        arch = self.arch.lower()
        if 'x86_64' in arch or 'amd64' in arch:
            return ARCH_64_BIT
        elif 'x86' in arch or 'i386' in arch or 'i686' in arch:
            return ARCH_32_BIT
        elif 'arm64' in arch or 'aarch64' in arch:
            return ARCH_ARM64
        else:
            return arch
    
    def check_compatibility(self) -> bool:
        """Check system compatibility.
        
        Returns:
            True if system is compatible, False otherwise.
        """
        # Check Python version
        if self.python_version < MIN_PYTHON_VERSION:
            min_ver = '.'.join(map(str, MIN_PYTHON_VERSION))
            current_ver = '.'.join(map(str, self.python_version))
            Terminal.print_error(f'Python version must be >= {min_ver}, got {current_ver}')
            return False
        
        # Check compiler
        if self.compiler == 'none':
            Terminal.print_error('No C compiler found')
            return False
        
        # Check CMake
        if self.cmake_version == 'Not found':
            Terminal.print_error('CMake not found')
            return False
        
        # Check CMake version
        try:
            version = Validators.validate_cmake_version(self.cmake_version.split()[-1])
        except ValidationError as e:
            Terminal.print_error(f'CMake version error: {e.message}')
            return False
        
        return True


class Configuration:
    """Build configuration.
    
    Stores all configuration options for the build process.
    """
    
    def __init__(self) -> None:
        """Initialize configuration with defaults."""
        self.build_type: str = DEFAULT_BUILD_TYPE
        self.allocator: str = DEFAULT_ALLOCATOR
        self.install_prefix: str = DEFAULT_INSTALL_PREFIX
        self.websocket: bool = True
        self.static_files: bool = True
        self.rate_limit: bool = True
        self.tls: bool = True
        self.build_tests: bool = True
        self.build_examples: bool = True
        self.build_benchmarks: bool = True
        self.verbose: bool = False
        self.parallel_jobs: int = os.cpu_count() or 1
    
    def to_dict(self) -> Dict[str, str]:
        """Convert configuration to dictionary.
        
        Returns:
            Configuration as dictionary with CMake flag keys.
        """
        allocator_map = {
            ALLOCATOR_SYSTEM: str(ALLOCATOR_TYPE_SYSTEM),
            ALLOCATOR_MIMALLOC: str(ALLOCATOR_TYPE_MIMALLOC),
            ALLOCATOR_CUSTOM: str(ALLOCATOR_TYPE_CUSTOM)
        }
        
        return {
            'CMAKE_BUILD_TYPE': self.build_type,
            'UVHTTP_ALLOCATOR_TYPE': allocator_map.get(self.allocator, str(ALLOCATOR_TYPE_SYSTEM)),
            'CMAKE_INSTALL_PREFIX': self.install_prefix,
            FEATURE_WEBSOCKET_FLAG: 'ON' if self.websocket else 'OFF',
            FEATURE_STATIC_FILES_FLAG: 'ON' if self.static_files else 'OFF',
            FEATURE_RATE_LIMIT_FLAG: 'ON' if self.rate_limit else 'OFF',
            FEATURE_TLS_FLAG: 'ON' if self.tls else 'OFF',
            BUILD_TESTS_FLAG: 'ON' if self.build_tests else 'OFF',
            BUILD_EXAMPLES_FLAG: 'ON' if self.build_examples else 'OFF',
            BUILD_BENCHMARKS_FLAG: 'ON' if self.build_benchmarks else 'OFF',
        }
    
    def generate_cmake_command(self) -> str:
        """Generate CMake build command.
        
        Returns:
            CMake command string.
        """
        build_dir = BUILD_DIR_NAME
        config_dict = self.to_dict()
        
        cmd_parts = ['mkdir', '-p', build_dir, '&&', 'cd', build_dir, '&&', 'cmake']
        
        for key, value in config_dict.items():
            cmd_parts.extend([f'-D{key}={value}'])
        
        cmd_parts.append('..')
        cmd_parts.extend(['&&', 'make', f'-j{self.parallel_jobs}'])
        
        return ' '.join(cmd_parts)


class ConfigurationWizard:
    """Interactive configuration wizard.
    
    Provides a step-by-step interface for configuring UVHTTP build.
    """
    
    def __init__(self, system_info: SystemInfo) -> None:
        """Initialize configuration wizard.
        
        Args:
            system_info: System information object.
        """
        self.system_info = system_info
        self.config = Configuration()
        self.steps = [
            STEP_SYSTEM_DETECTION,
            STEP_BUILD_CONFIG,
            STEP_MEMORY_ALLOCATOR,
            STEP_FEATURE_MODULES,
            STEP_BUILD_COMPONENTS,
            STEP_ADVANCED_OPTIONS,
            STEP_CONFIG_SUMMARY,
            STEP_BUILD_COMMAND
        ]
        self.current_step: int = 1
    
    def print_progress(self) -> None:
        """Print configuration progress."""
        progress = int((self.current_step / len(self.steps)) * 100)
        Terminal.print_progress(self.current_step, len(self.steps))
        print()
    
    def run(self) -> bool:
        """Run the configuration wizard.
        
        Returns:
            True if configuration completed successfully, False otherwise.
        """
        try:
            Terminal.print_header('UVHTTP Configuration Tool')
            
            # System detection
            self._system_detection_step()
            self.current_step += 1
            self.print_progress()
            
            # Build configuration
            self._build_config_step()
            self.current_step += 1
            self.print_progress()
            
            # Memory allocator
            self._memory_allocator_step()
            self.current_step += 1
            self.print_progress()
            
            # Feature modules
            self._feature_modules_step()
            self.current_step += 1
            self.print_progress()
            
            # Build components
            self._build_components_step()
            self.current_step += 1
            self.print_progress()
            
            # Advanced options
            self._advanced_options_step()
            self.current_step += 1
            self.print_progress()
            
            # Configuration summary
            self._config_summary_step()
            self.current_step += 1
            self.print_progress()
            
            # Build command
            self._build_command_step()
            self.current_step += 1
            self.print_progress()
            
            Terminal.print_success(SUCCESS_CONFIG_COMPLETE)
            return True
            
        except KeyboardInterrupt:
            Terminal.print_warning('\nConfiguration cancelled by user')
            return False
        except ValidationError as e:
            Terminal.print_error(f'Validation error: {e.message}')
            return False
        except Exception as e:
            Terminal.print_error(f'Unexpected error: {e}')
            return False
    
    def _system_detection_step(self) -> None:
        """System detection step."""
        Terminal.print_section(STEP_SYSTEM_DETECTION)
        
        Terminal.print_key_value('OS', self.system_info.os_name)
        Terminal.print_key_value('Architecture', self.system_info.arch)
        Terminal.print_key_value('CPU Cores', str(self.system_info.cpu_count))
        Terminal.print_key_value('Compiler', self.system_info.compiler)
        Terminal.print_key_value('CMake', self.system_info.cmake_version)
        Terminal.print_key_value('Python', '.'.join(map(str, self.system_info.python_version)))
    
    def _build_config_step(self) -> None:
        """Build configuration step."""
        Terminal.print_section(STEP_BUILD_CONFIG)
        
        # Build type
        Terminal.print_question('Build type (Debug/Release/RelWithDebInfo/MinSizeRel) [Release]:')
        choice = input().strip() or DEFAULT_BUILD_TYPE
        try:
            self.config.build_type = Validators.validate_build_type(choice)
        except ValidationError as e:
            Terminal.print_warning(f'Using default: {DEFAULT_BUILD_TYPE}')
            self.config.build_type = DEFAULT_BUILD_TYPE
        
        # Installation prefix
        Terminal.print_question(f'Installation prefix [{DEFAULT_INSTALL_PREFIX}]:')
        prefix = input().strip() or DEFAULT_INSTALL_PREFIX
        try:
            path = Validators.validate_install_prefix(prefix)
            self.config.install_prefix = str(path)
        except ValidationError as e:
            Terminal.print_warning(f'Using default: {DEFAULT_INSTALL_PREFIX}')
            self.config.install_prefix = DEFAULT_INSTALL_PREFIX
    
    def _memory_allocator_step(self) -> None:
        """Memory allocator selection step."""
        Terminal.print_section(STEP_MEMORY_ALLOCATOR)
        
        options = [
            'system - System allocator (default)',
            'mimalloc - High-performance allocator (recommended for 64-bit)',
            'custom - Custom allocator (requires implementation)'
        ]
        
        for i, option in enumerate(options, 1):
            Terminal.print_option(i, option)
        
        Terminal.print_question('Select allocator [2]:')
        choice = input().strip() or '2'
        try:
            index = Validators.validate_menu_choice(choice, options)
            self.config.allocator = [ALLOCATOR_SYSTEM, ALLOCATOR_MIMALLOC, ALLOCATOR_CUSTOM][index]
        except ValidationError:
            Terminal.print_warning(f'Using default: {DEFAULT_ALLOCATOR}')
            self.config.allocator = DEFAULT_ALLOCATOR
    
    def _feature_modules_step(self) -> None:
        """Feature modules selection step."""
        Terminal.print_section(STEP_FEATURE_MODULES)
        
        # WebSocket
        Terminal.print_question('Enable WebSocket support? [Y/n]:')
        choice = input().strip() or 'y'
        try:
            self.config.websocket = Validators.validate_yes_no(choice)
        except ValidationError:
            self.config.websocket = True
        
        # Static files
        Terminal.print_question('Enable static file support? [Y/n]:')
        choice = input().strip() or 'y'
        try:
            self.config.static_files = Validators.validate_yes_no(choice)
        except ValidationError:
            self.config.static_files = True
        
        # Rate limiting
        Terminal.print_question('Enable rate limiting? [Y/n]:')
        choice = input().strip() or 'y'
        try:
            self.config.rate_limit = Validators.validate_yes_no(choice)
        except ValidationError:
            self.config.rate_limit = True
        
        # TLS
        Terminal.print_question('Enable TLS support? [Y/n]:')
        choice = input().strip() or 'y'
        try:
            self.config.tls = Validators.validate_yes_no(choice)
        except ValidationError:
            self.config.tls = True
    
    def _build_components_step(self) -> None:
        """Build components selection step."""
        Terminal.print_section(STEP_BUILD_COMPONENTS)
        
        # Tests
        Terminal.print_question('Build tests? [Y/n]:')
        choice = input().strip() or 'y'
        try:
            self.config.build_tests = Validators.validate_yes_no(choice)
        except ValidationError:
            self.config.build_tests = True
        
        # Examples
        Terminal.print_question('Build examples? [Y/n]:')
        choice = input().strip() or 'y'
        try:
            self.config.build_examples = Validators.validate_yes_no(choice)
        except ValidationError:
            self.config.build_examples = True
        
        # Benchmarks
        Terminal.print_question('Build benchmarks? [Y/n]:')
        choice = input().strip() or 'y'
        try:
            self.config.build_benchmarks = Validators.validate_yes_no(choice)
        except ValidationError:
            self.config.build_benchmarks = True
    
    def _advanced_options_step(self) -> None:
        """Advanced options step."""
        Terminal.print_section(STEP_ADVANCED_OPTIONS)
        
        # Parallel jobs
        Terminal.print_question(f'Parallel jobs [{self.system_info.cpu_count}]:')
        choice = input().strip() or str(self.system_info.cpu_count)
        try:
            self.config.parallel_jobs = Validators.validate_jobs(choice, self.system_info.cpu_count * 2)
        except ValidationError as e:
            Terminal.print_warning(f'{e.message}, using default: {self.system_info.cpu_count}')
            self.config.parallel_jobs = self.system_info.cpu_count
        
        # Verbose output
        Terminal.print_question('Enable verbose output? [y/N]:')
        choice = input().strip() or 'n'
        try:
            self.config.verbose = Validators.validate_yes_no(choice)
        except ValidationError:
            self.config.verbose = False
    
    def _config_summary_step(self) -> None:
        """Configuration summary step."""
        Terminal.print_section(STEP_CONFIG_SUMMARY)
        
        print(f"  Build Type: {Colors.CYAN}{self.config.build_type}{Colors.RESET}")
        print(f"  Allocator: {Colors.CYAN}{self.config.allocator}{Colors.RESET}")
        print(f"  Install Prefix: {Colors.CYAN}{self.config.install_prefix}{Colors.RESET}")
        print(f"  WebSocket: {Colors.CYAN}{self.config.websocket}{Colors.RESET}")
        print(f"  Static Files: {Colors.CYAN}{self.config.static_files}{Colors.RESET}")
        print(f"  Rate Limit: {Colors.CYAN}{self.config.rate_limit}{Colors.RESET}")
        print(f"  TLS: {Colors.CYAN}{self.config.tls}{Colors.RESET}")
        print(f"  Build Tests: {Colors.CYAN}{self.config.build_tests}{Colors.RESET}")
        print(f"  Build Examples: {Colors.CYAN}{self.config.build_examples}{Colors.RESET}")
        print(f"  Build Benchmarks: {Colors.CYAN}{self.config.build_benchmarks}{Colors.RESET}")
        print(f"  Parallel Jobs: {Colors.CYAN}{self.config.parallel_jobs}{Colors.RESET}")
        print(f"  Verbose: {Colors.CYAN}{self.config.verbose}{Colors.RESET}")
    
    def _build_command_step(self) -> None:
        """Build command step."""
        Terminal.print_section(STEP_BUILD_COMMAND)
        
        cmd = self.config.generate_cmake_command()
        
        Terminal.print_info('Generated CMake command:')
        print()
        Terminal.print_box(cmd)
        print()
        
        Terminal.print_question('Run build now? [y/N]:')
        choice = input().strip() or 'n'
        try:
            if Validators.validate_yes_no(choice):
                Terminal.print_info('Running build...')
                try:
                    # Use a reasonable timeout for build (30 minutes)
                    result = subprocess.run(
                        cmd,
                        shell=True,
                        check=True,
                        timeout=1800,
                        capture_output=True,
                        text=True
                    )
                    Terminal.print_success('Build completed successfully')
                    if result.stdout:
                        print(result.stdout[-500:])  # Show last 500 chars of output
                except subprocess.CalledProcessError as e:
                    error_msg = f'Build failed with exit code {e.returncode}'
                    if e.stderr:
                        error_msg += f'\nError output:\n{e.stderr[-500:]}'
                    Terminal.print_error(error_msg)
                    raise
                except subprocess.TimeoutExpired:
                    Terminal.print_error('Build timed out after 30 minutes')
                    raise
                except PermissionError:
                    Terminal.print_error('Permission denied: Cannot execute build command')
                    raise
                except FileNotFoundError as e:
                    Terminal.print_error(f'Build command not found: {e.filename}')
                    raise
                except OSError as e:
                    Terminal.print_error(f'Build system error: {e}')
                    raise
        except ValidationError:
            Terminal.print_info('Build command saved. Run manually to build.')


def main() -> int:
    """Main entry point.
    
    Returns:
        Exit code.
    """
    parser = argparse.ArgumentParser(
        description='UVHTTP Configuration Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        '--version',
        action='version',
        version='UVHTTP Configuration Tool 2.0'
    )
    parser.add_argument(
        '--check',
        action='store_true',
        help='Check system compatibility only'
    )
    
    args = parser.parse_args()
    
    # Detect system information
    system_info = SystemInfo()
    
    # Check compatibility
    if not system_info.check_compatibility():
        return EXIT_ERROR
    
    # Only check compatibility
    if args.check:
        Terminal.print_success('System is compatible')
        return EXIT_SUCCESS
    
    # Run configuration wizard
    wizard = ConfigurationWizard(system_info)
    if wizard.run():
        return EXIT_SUCCESS
    else:
        return EXIT_ERROR


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print()
        sys.exit(EXIT_ERROR)
    except Exception as e:
        Terminal.print_error(f'Fatal error: {e}')
        sys.exit(EXIT_ERROR)
