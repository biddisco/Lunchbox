
include $(TOP)/make/system.mk

SUBDIRS = \
	base \
	client \
	compositor \
	connection \
	dataStream \
	image \
	netperf \
	node \
	pipeperf \
	session \
	wall

TARGETS = subdirs

include $(TOP)/make/rules.mk
