CONTIKI = /home/contiki/contiki-ng

SHELL := /bin/bash


patches+=cc1310-config.patch
patches+=cc1310-spi-slave.patch

dir=$(shell pwd)

patch: 
	@for patch in $(patches); \
		do \
			echo Checking patch $$patch ; \
			cd $(CONTIKI) ; \
			if ! ( patch -R -p1 -s -f --dry-run < $(dir)/$$patch ) > /dev/null ; then \
				echo Applying $(dir)/$$patch ; \
				patch -p1 < $(dir)/$$patch ; \
			fi  \
		done 
