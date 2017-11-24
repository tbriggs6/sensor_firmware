FLAGS=-g
TARGET=srf06-cc26xx
BOARD=launchpad/cc1310

all: edgeNode sensorNode testNode

.PHONY: clean
clean:
	cd edgeNode && make clean
	cd sensorNode && make clean
	cd testNode && make clean

.PHONY: edgeNode
edgeNode:
	cd edgeNode && make  TARGET=$(TARGET) BOARD=$(BOARD)

.PHONY: sensorNode
sensorNode:
	cd sensorNode && make TARGET=$(TARGET) BOARD=$(BOARD)
	
.PHONY: testNode
testNode:
	cd testNode && make TARGET=$(TARGET) BOARD=$(BOARD)
