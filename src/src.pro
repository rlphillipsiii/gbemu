include(build.pri)

CONFIG += ordered

TEMPLATE = subdirs

SUBDIRS += utility
SUBDIRS += hardware
SUBDIRS += ui

unix: SUBDIRS += server

#SUBDIRS += test
