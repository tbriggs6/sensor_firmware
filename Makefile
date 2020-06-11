CONTIKI = /home/contiki/contiki-ng

SUBPRJS = $(dir $(wildcard */Makefile))

ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif


all:
	for dir in $(SUBPRJS); do \
		(cd $$dir; make TARGET=$(TARGET) BOARD=$(BOARD) ) 	\
	done
		   
clean:
	for dir in $(SUBPRJS); do \
		(cd $$dir; make TARGET=$(TARGET) BOARD=$(BOARD) clean) 	\
	done		 