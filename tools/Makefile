
MKFS_DIR := mkfs
SYM_DIR := sym


.PHONY: mkfs sym
mkfs:
	$(MAKE) -C $(MKFS_DIR) mkfs DISK=$(DISK) OUT=$(OUT)

mkfs_clean:
	$(MAKE) -C $(MKFS_DIR) clean DISK=$(DISK) OUT=$(OUT)
	
sym:
	$(MAKE) -C $(SYM_DIR) sym KERNEL=$(KERNEL)

sym_clean:
	$(MAKE) -C $(SYM_DIR) clean
