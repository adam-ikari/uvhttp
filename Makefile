# UVHTTP Makefile
# Default makefile that delegates to GNUmakefile
# All targets are defined in GNUmakefile

.PHONY: help
.DEFAULT_GOAL := help

# Default target - show help and delegate to GNUmakefile
help:
	@$(MAKE) -f GNUmakefile help

# Delegate all other targets to GNUmakefile
%:
	@$(MAKE) -f GNUmakefile $@
