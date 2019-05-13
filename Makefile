pavolcli: pavolcli.c
	$(CC) -O2 $(CCFLAGS) $< -lpulse -o $@
