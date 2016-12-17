ARMBOOT = armboot
NSWITCH = nswitch/boot.dol
ARMBOOT-DEPS = armboot/*.* armboot/stub/* armboot/stubsb1/* armboot/elfloader/*
NSWITCH-DEPS = nswitch/source/*

all: boot.dol

boot.dol: $(ARMBOOT-DEPS) $(NSWITCH-DEPS)
	@make -C $(ARMBOOT)
	@make -C nswitch
	@cp $(NSWITCH) ./

clean:
	@make -C $(ARMBOOT) clean
	@make -C nswitch clean
	@rm boot.dol