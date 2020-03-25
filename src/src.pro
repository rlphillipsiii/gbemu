include(build.pri)

CONFIG += ordered

TEMPLATE = subdirs

SUBDIRS += utility
SUBDIRS += hardware
SUBDIRS += server
SUBDIRS += ui

#SUBDIRS += test
