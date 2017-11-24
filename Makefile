FLAGS=-g
TARGET=srf06-cc13xx
#BOARD=watersensor/cc1310
BOARD=launchpad/cc1310

all: edgeNode sensorNode

.PHONY: clean
clean:
	cd edgeNode && make clean
	cd sensorNode && make clean

.PHONY: edgeNode
edgeNode:
	cd edgeNode && make 

.PHONY: sensorNode
sensorNode:
	cd sensorNode && make
