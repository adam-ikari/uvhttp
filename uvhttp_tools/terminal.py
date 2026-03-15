"""Terminal utilities for UVHTTP configuration tools."""

import os
import sys
import shutil
from typing import Optional

from .constants import (
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
    COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE, COLOR_RESET,
    STYLE_BOLD, STYLE_DIM, STYLE_UNDERLINE,
    DEFAULT_TERMINAL_WIDTH, ENV_NO_COLOR, ENV_TERM
)


class Colors:
    """ANSI color codes for terminal output.
    
    This class provides static methods and attributes for colorizing
    terminal output. Colors can be disabled for non-TTY output or
    when NO_COLOR environment variable is set.
    """
    
    # ANSI color codes
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    WHITE = '\033[37m'
    RESET = '\033[0m'
    
    # Text styles
    BOLD = '\033[1m'
    DIM = '\033[2m'
    UNDERLINE = '\033[4m'
    BLINK = '\033[5m'
    REVERSE = '\033[7m'
    
    # Bright colors
    BRIGHT_RED = '\033[91m'
    BRIGHT_GREEN = '\033[92m'
    BRIGHT_YELLOW = '\033[93m'
    BRIGHT_BLUE = '\033[94m'
    BRIGHT_MAGENTA = '\033[95m'
    BRIGHT_CYAN = '\033[96m'
    BRIGHT_WHITE = '\033[97m'
    
    # Background colors
    BG_RED = '\033[41m'
    BG_GREEN = '\033[42m'
    BG_YELLOW = '\033[43m'
    BG_BLUE = '\033[44m'
    BG_MAGENTA = '\033[45m'
    BG_CYAN = '\033[46m'
    BG_WHITE = '\033[47m'
    
    _disabled: bool = False
    
    @classmethod
    def disable(cls) -> None:
        """Disable all colors.
        
        This is useful for non-TTY output or when colors are not supported.
        """
        cls._disabled = True
        cls.RED = ''
        cls.GREEN = ''
        cls.YELLOW = ''
        cls.BLUE = ''
        cls.MAGENTA = ''
        cls.CYAN = ''
        cls.WHITE = ''
        cls.RESET = ''
        cls.BOLD = ''
        cls.DIM = ''
        cls.UNDERLINE = ''
        cls.BLINK = ''
        cls.REVERSE = ''
        cls.BRIGHT_RED = ''
        cls.BRIGHT_GREEN = ''
        cls.BRIGHT_YELLOW = ''
        cls.BRIGHT_BLUE = ''
        cls.BRIGHT_MAGENTA = ''
        cls.BRIGHT_CYAN = ''
        cls.BRIGHT_WHITE = ''
        cls.BG_RED = ''
        cls.BG_GREEN = ''
        cls.BG_YELLOW = ''
        cls.BG_BLUE = ''
        cls.BG_MAGENTA = ''
        cls.BG_CYAN = ''
        cls.BG_WHITE = ''
    
    @classmethod
    def enable_if_tty(cls) -> None:
        """Enable colors only if running in a TTY.
        
        This checks if stdout is a TTY and if colors are supported.
        Colors are disabled if:
        - Not running in a TTY
        - NO_COLOR environment variable is set
        - TERM is 'dumb' or 'emacs'
        """
        # Check if running in TTY
        if not sys.stdout.isatty():
            cls.disable()
            return
        
        # Check NO_COLOR environment variable
        if os.environ.get(ENV_NO_COLOR):
            cls.disable()
            return
        
        # Check if terminal supports colors
        term = os.environ.get(ENV_TERM, '')
        if term in ['dumb', 'emacs']:
            cls.disable()
            return


class Terminal:
    """Terminal utility class for formatted output.
    
    This class provides methods for printing formatted messages
    with colors, styles, and consistent formatting.
    """
    
    @staticmethod
    def clear_screen() -> None:
        """Clear the terminal screen.
        
        This works on both Unix-like systems and Windows.
        """
        if os.name == 'nt':
            os.system('cls')
        else:
            os.system('clear')
    
    @staticmethod
    def get_width() -> int:
        """Get terminal width with fallback.
        
        Returns:
            Terminal width in columns, or DEFAULT_TERMINAL_WIDTH if detection fails.
        """
        try:
            return shutil.get_terminal_size().columns
        except (OSError, ValueError):
            return DEFAULT_TERMINAL_WIDTH
    
    @staticmethod
    def print_header(title: str, width: Optional[int] = None) -> None:
        """Print a formatted header.
        
        Args:
            title: Header title text.
            width: Optional width for header. If not provided, uses terminal width.
        """
        if width is None:
            width = Terminal.get_width()
        
        line = '═' * width
        padding = (width - len(title) - 2) // 2
        
        print(f"\n{line}")
        print(f"{' ' * padding}{title}")
        print(f"{line}\n")
    
    @staticmethod
    def print_section(title: str) -> None:
        """Print a section header.
        
        Args:
            title: Section title text.
        """
        print(f"\n{Colors.BOLD}{Colors.CYAN}{title}{Colors.RESET}")
        print(f"{Colors.DIM}{'─' * Terminal.get_width()}{Colors.RESET}\n")
    
    @staticmethod
    def print_success(message: str) -> None:
        """Print a success message.
        
        Args:
            message: Success message text.
        """
        print(f"{Colors.BRIGHT_GREEN}✓{Colors.RESET} {message}")
    
    @staticmethod
    def print_error(message: str) -> None:
        """Print an error message.
        
        Args:
            message: Error message text.
        """
        print(f"{Colors.BRIGHT_RED}✗{Colors.RESET} {Colors.RED}{message}{Colors.RESET}")
    
    @staticmethod
    def print_warning(message: str) -> None:
        """Print a warning message.
        
        Args:
            message: Warning message text.
        """
        print(f"{Colors.BRIGHT_YELLOW}⚠{Colors.RESET} {Colors.YELLOW}{message}{Colors.RESET}")
    
    @staticmethod
    def print_info(message: str) -> None:
        """Print an informational message.
        
        Args:
            message: Information message text.
        """
        print(f"{Colors.BRIGHT_CYAN}ℹ{Colors.RESET} {Colors.CYAN}{message}{Colors.RESET}")
    
    @staticmethod
    def print_question(prompt: str) -> None:
        """Print a question prompt.
        
        Args:
            prompt: Question text.
        """
        print(f"{Colors.BOLD}{Colors.MAGENTA}{prompt}{Colors.RESET} ", end='')
    
    @staticmethod
    def print_option(index: int, text: str, selected: bool = False) -> None:
        """Print a menu option.
        
        Args:
            index: Option index (1-based).
            text: Option text.
            selected: Whether this option is selected.
        """
        if selected:
            prefix = f"{Colors.BRIGHT_GREEN}►{Colors.RESET}"
        else:
            prefix = f"{Colors.DIM}•{Colors.RESET}"
        
        print(f"  {prefix} {Colors.BOLD}{index}.{Colors.RESET} {text}")
    
    @staticmethod
    def print_key_value(key: str, value: str, width: int = 20) -> None:
        """Print a key-value pair with consistent formatting.
        
        Args:
            key: Key text.
            value: Value text.
            width: Width for key column.
        """
        print(f"  {Colors.BOLD}{key:<{width}}{Colors.RESET} [{Colors.CYAN}{value}{Colors.RESET}]")
    
    @staticmethod
    def print_progress(current: int, total: int, message: str = '') -> None:
        """Print a progress bar.
        
        Args:
            current: Current progress value.
            total: Total progress value.
            message: Optional message to display.
        """
        progress = int((current / total) * 100)
        filled = progress // 5
        empty = 20 - filled
        
        bar = f"{Colors.BRIGHT_GREEN}{'█' * filled}{Colors.DIM}{'░' * empty}{Colors.RESET}"
        
        if message:
            print(f"\r{Colors.BOLD}{Colors.CYAN}Progress:{Colors.RESET} [{bar}] {progress}% - {message}", 
                  end='', flush=True)
        else:
            print(f"\r{Colors.BOLD}{Colors.CYAN}Progress:{Colors.RESET} [{bar}] {progress}%", 
                  end='', flush=True)
    
    @staticmethod
    def print_box(text: str, width: Optional[int] = None) -> None:
        """Print text in a box.
        
        Args:
            text: Text to display in box.
            width: Optional width for box. If not provided, uses terminal width.
        """
        if width is None:
            width = Terminal.get_width()
        
        lines = text.split('\n')
        max_line_len = max(len(line) for line in lines)
        box_width = min(max_line_len + 4, width - 4)
        
        print(f"{Colors.BOLD}{Colors.CYAN}╔{'═' * (box_width - 2)}╗{Colors.RESET}")
        for line in lines:
            padding = (box_width - len(line) - 2) // 2
            print(f"{Colors.BOLD}{Colors.CYAN}║{Colors.RESET} {' ' * padding}{line}{' ' * (box_width - len(line) - padding - 2)} {Colors.BOLD}{Colors.CYAN}║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}╚{'═' * (box_width - 2)}╝{Colors.RESET}")


# Initialize colors based on terminal support
Colors.enable_if_tty()
