CONTIKI = /home/contiki/contiki-ng

SUBPRJS = $(dir $(wildcard */Makefile))

mrclean:
	for DIR in $(SUBPRJS); do \
		rm -rf $$DIR/build;  \
	done
