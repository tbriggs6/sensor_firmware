#!/bin/bash


if ! patch -R -p0 -s -f --dry-run < $<; then \
  		patch -p1 < $< \
	fi \
