"""Input validation utilities for UVHTTP configuration tools."""

import os
import re
from pathlib import Path
from typing import Optional, List, Tuple

from .constants import (
    VALID_INSTALL_PREFIX_PATTERN, VALID_VERSION_PATTERN,
    ERROR_INVALID_INPUT, VALIDATION_ERROR_EMPTY,
    VALIDATION_ERROR_INVALID_FORMAT, VALIDATION_ERROR_OUT_OF_RANGE,
    VALIDATION_ERROR_NOT_FOUND, VALIDATION_ERROR_PERMISSION,
    MIN_CMAKE_VERSION, ALLOCATOR_SYSTEM, ALLOCATOR_MIMALLOC, ALLOCATOR_CUSTOM
)

from .terminal import Terminal


class ValidationError(Exception):
    """Exception raised for validation errors."""
    
    def __init__(self, message: str, field: Optional[str] = None):
        """Initialize validation error.
        
        Args:
            message: Error message.
            field: Optional field name that failed validation.
        """
        self.message = message
        self.field = field
        super().__init__(self.message)


class Validators:
    """Input validation utilities.
    
    This class provides static methods for validating various types of
    user input to ensure security and correctness.
    """
    
    # Whitelist of allowed characters for installation prefix
    ALLOWED_PREFIX_CHARS = set('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-/.')
    
    # Valid build types
    VALID_BUILD_TYPES = ['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel']
    
    # Valid allocator types
    VALID_ALLOCATORS = [ALLOCATOR_SYSTEM, ALLOCATOR_MIMALLOC, ALLOCATOR_CUSTOM]
    
    @staticmethod
    def validate_install_prefix(prefix: str) -> Path:
        """Validate and sanitize installation prefix path.
        
        This prevents path traversal attacks and ensures the path is safe.
        
        Args:
            prefix: Installation prefix path string.
        
        Returns:
            Sanitized Path object.
        
        Raises:
            ValidationError: If the path is invalid or unsafe.
        """
        if not prefix:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'install_prefix')
        
        # Check for path traversal
        if '..' in prefix:
            raise ValidationError('Path traversal detected', 'install_prefix')
        
        # Check for suspicious characters
        suspicious_chars = ['\x00', '\n', '\r', '\t', ';', '&', '|', '$', '`', '(', ')', '<', '>']
        for char in suspicious_chars:
            if char in prefix:
                raise ValidationError(f'Invalid character in path: {repr(char)}', 'install_prefix')
        
        try:
            path = Path(prefix).expanduser().resolve()
        except (OSError, ValueError) as e:
            raise ValidationError(f'Invalid path: {e}', 'install_prefix')
        
        # Check if parent directory exists
        if not path.parent.exists():
            raise ValidationError(f'Parent directory does not exist: {path.parent}', 'install_prefix')
        
        # Check if parent directory is writable
        if not os.access(path.parent, os.W_OK):
            raise ValidationError(f'Parent directory is not writable: {path.parent}', 'install_prefix')
        
        # Check for suspicious system directories
        suspicious_dirs = ['/dev', '/proc', '/sys', '/etc', '/boot', '/root']
        if any(str(path).startswith(d) for d in suspicious_dirs):
            raise ValidationError(f'Suspicious install prefix: {path}', 'install_prefix')
        
        return path
    
    @staticmethod
    def validate_build_type(build_type: str) -> str:
        """Validate build type.
        
        Args:
            build_type: Build type string.
        
        Returns:
            Validated build type.
        
        Raises:
            ValidationError: If build type is invalid.
        """
        if not build_type:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'build_type')
        
        if build_type not in Validators.VALID_BUILD_TYPES:
            raise ValidationError(
                f'Invalid build type. Must be one of: {", ".join(Validators.VALID_BUILD_TYPES)}',
                'build_type'
            )
        
        return build_type
    
    @staticmethod
    def validate_allocator(allocator: str) -> str:
        """Validate memory allocator type.
        
        Args:
            allocator: Allocator type string.
        
        Returns:
            Validated allocator type.
        
        Raises:
            ValidationError: If allocator type is invalid.
        """
        if not allocator:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'allocator')
        
        if allocator not in Validators.VALID_ALLOCATORS:
            raise ValidationError(
                f'Invalid allocator. Must be one of: {", ".join(Validators.VALID_ALLOCATORS)}',
                'allocator'
            )
        
        return allocator
    
    @staticmethod
    def validate_port(port_str: str) -> int:
        """Validate port number.
        
        Args:
            port_str: Port number as string.
        
        Returns:
            Validated port number as integer.
        
        Raises:
            ValidationError: If port number is invalid.
        """
        if not port_str:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'port')
        
        # Check for non-numeric characters
        if not port_str.isdigit():
            raise ValidationError('Port must be a number', 'port')
        
        port = int(port_str)
        
        if port < 1 or port > 65535:
            raise ValidationError(VALIDATION_ERROR_OUT_OF_RANGE, 'port')
        
        # Prevent well-known system ports (0-1023) unless running as root
        if port < 1024 and os.geteuid() != 0:
            raise ValidationError('Port must be >= 1024 (requires root for privileged ports)', 'port')
        
        return port
    
    @staticmethod
    def validate_cmake_version(version_str: str) -> Tuple[int, int, int]:
        """Validate CMake version string.
        
        Args:
            version_str: CMake version string (e.g., "3.20.0").
        
        Returns:
            Tuple of version components (major, minor, patch).
        
        Raises:
            ValidationError: If version string is invalid.
        """
        if not version_str:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'cmake_version')
        
        # Match version pattern
        match = re.match(r'^(\d+)\.(\d+)\.(\d+)', version_str)
        if not match:
            raise ValidationError(VALIDATION_ERROR_INVALID_FORMAT, 'cmake_version')
        
        major, minor, patch = int(match.group(1)), int(match.group(2)), int(match.group(3))
        version = (major, minor, patch)
        
        # Check minimum version requirement
        if version < MIN_CMAKE_VERSION:
            min_version_str = '.'.join(map(str, MIN_CMAKE_VERSION))
            raise ValidationError(
                f'CMake version must be >= {min_version_str}, got {version_str}',
                'cmake_version'
            )
        
        return version
    
    @staticmethod
    def validate_jobs(jobs_str: str, max_jobs: Optional[int] = None) -> int:
        """Validate number of parallel jobs.
        
        Args:
            jobs_str: Number of jobs as string.
            max_jobs: Optional maximum number of jobs.
        
        Returns:
            Validated number of jobs.
        
        Raises:
            ValidationError: If jobs value is invalid.
        """
        if not jobs_str:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'jobs')
        
        # Check for non-numeric characters
        if not jobs_str.isdigit():
            raise ValidationError('Jobs must be a positive integer', 'jobs')
        
        jobs = int(jobs_str)
        
        if jobs < 1:
            raise ValidationError('Jobs must be >= 1', 'jobs')
        
        if max_jobs and jobs > max_jobs:
            raise ValidationError(f'Jobs must be <= {max_jobs}', 'jobs')
        
        return jobs
    
    @staticmethod
    def validate_yes_no(input_str: str) -> bool:
        """Validate yes/no input.
        
        Args:
            input_str: User input string.
        
        Returns:
            True for yes, False for no.
        
        Raises:
            ValidationError: If input is not yes or no.
        """
        if not input_str:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'yes_no')
        
        input_lower = input_str.lower().strip()
        
        if input_lower in ['y', 'yes', 'true', '1', 'on']:
            return True
        elif input_lower in ['n', 'no', 'false', '0', 'off']:
            return False
        else:
            raise ValidationError('Please enter yes or no', 'yes_no')
    
    @staticmethod
    def validate_menu_choice(choice_str: str, options: List[str]) -> int:
        """Validate menu choice.
        
        Args:
            choice_str: User choice as string.
            options: List of available options.
        
        Returns:
            Validated choice index (0-based).
        
        Raises:
            ValidationError: If choice is invalid.
        """
        if not choice_str:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'choice')
        
        # Check for non-numeric characters
        if not choice_str.isdigit():
            raise ValidationError('Please enter a number', 'choice')
        
        choice = int(choice_str)
        
        if choice < 1 or choice > len(options):
            raise ValidationError(
                f'Please enter a number between 1 and {len(options)}',
                'choice'
            )
        
        return choice - 1  # Convert to 0-based
    
    @staticmethod
    def sanitize_shell_string(input_str: str) -> str:
        """Sanitize string for safe shell usage.
        
        This removes potentially dangerous characters that could be used
        for shell injection attacks.
        
        Args:
            input_str: Input string to sanitize.
        
        Returns:
            Sanitized string.
        
        Raises:
            ValidationError: If string contains dangerous characters.
        """
        if not input_str:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'input')
        
        # Dangerous characters
        dangerous_chars = [';', '&', '|', '$', '`', '(', ')', '<', '>', '\\', '\n', '\r', '\t']
        
        for char in dangerous_chars:
            if char in input_str:
                raise ValidationError(
                    f'Invalid character in input: {repr(char)}',
                    'input'
                )
        
        # Remove null bytes
        input_str = input_str.replace('\x00', '')
        
        # Limit length
        if len(input_str) > 1000:
            raise ValidationError('Input too long (max 1000 characters)', 'input')
        
        return input_str.strip()
    
    @staticmethod
    def validate_directory_path(path_str: str, must_exist: bool = False, must_be_writable: bool = False) -> Path:
        """Validate directory path.
        
        Args:
            path_str: Directory path string.
            must_exist: Whether directory must exist.
            must_be_writable: Whether directory must be writable.
        
        Returns:
            Validated Path object.
        
        Raises:
            ValidationError: If path is invalid.
        """
        if not path_str:
            raise ValidationError(VALIDATION_ERROR_EMPTY, 'directory')
        
        try:
            path = Path(path_str).expanduser().resolve()
        except (OSError, ValueError) as e:
            raise ValidationError(f'Invalid path: {e}', 'directory')
        
        if must_exist and not path.exists():
            raise ValidationError(f'Directory does not exist: {path}', 'directory')
        
        if must_exist and not path.is_dir():
            raise ValidationError(f'Not a directory: {path}', 'directory')
        
        if must_be_writable and path.exists():
            if not os.access(path, os.W_OK):
                raise ValidationError(f'Directory is not writable: {path}', 'directory')
        
        return path
