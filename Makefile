.PHONY:
.DEFAULT_GOAL := help
%:
	@$(MAKE) -f GNUmakefile $@
