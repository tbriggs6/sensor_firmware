FLAGS=-g
TARGET=srf06-cc13xx
#BOARD=watersensor/cc1310
BOARD=launchpad/cc1310

all: edgeNode sensorNode testNode

.PHONY: clean
clean:
	cd edgeNode && make clean
	cd sensorNode && make clean
	cd testNode && make clean
	
.PHONY: edgeNode
edgeNode:
	cd edgeNode && make 

.PHONY: testNode
edgeNode:
	cd testNode && make

.PHONY: sensorNode
sensorNode:
	cd sensorNode && make
