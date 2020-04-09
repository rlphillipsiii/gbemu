include(build.pri)

CONFIG += ordered

TEMPLATE = subdirs

SUBDIRS += utility
SUBDIRS += hardware
SUBDIRS += ipc
SUBDIRS += ui

SERVER=$$(SERVER_BUILD)
!isEmpty(SERVER) {
    unix: SUBDIRS += server
}

#SUBDIRS += test
