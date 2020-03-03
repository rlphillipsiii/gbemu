CONFIG (release, debug|release) {
    TARGET_SPEC = release

    DEFINES += NDEBUG
    DEFINES += RELEASE

    QMAKE_CXXFLAGS += -O3
    QMAKE_LFLAGS += -O3
}

CONFIG (debug, debug|release) {
    TARGET_SPEC = debug

    DEFINES += DEBUG

    QMAKE_CXXFLAGS += -g
    QMAKE_CXXFLAGS += -O0

    QMAKE_CXXFLAGS += -fsanitize=address

    QMAKE_LFLAGS += -fsanitize=address
}

defineReplace(combine) {
    return($$join(ARGS, ""))
}

defineTest(createDirectory) {
    !isEmpty(ARGS) {
	    !exists($$ARGS) {
	        cmd = $$quote(mkdir -p "\"$$ARGS\"")
	        system($$cmd)
	    }
    }
}

defineTest(copyFile) {
    system(cp $$ARGS)
}

defineTest(publishFile) {
    message($$ARGS)
    system(mv $$ARGS)
}

defineTest(publishTarget) {
    destination = $$PUBLIC_BIN
    contains (TEMPLATE, lib) {
        destination = $$PUBLIC_LIB
    }

    #publishFile($$join($$TARGET_SPEC, $$TARGET, "/"), $$join($$list($$destination, $$TARGET), "/"))
}

defineTest(publishHeaders) {
    fileslist = $$ARGS

    for (target, fileslist) {
        exists($$target) {
            copyFile($$target, $$join($$list($$PUBLIC_INC, $$target), "/"))
        }
    }
}

defineReplace(getBaseDirectory) {
    needle = "config.py"

    exists($$needle) {
        return($$system("python $$needle -p"))
    }
    needle = $$combine("../", $$needle)
    exists($$needle) {
        return($$system("python $$needle -p"))
    }
    needle = $$combine("../", $$needle)
    exists($$needle) {
        return($$system("python $$needle -p"))
    }
    needle = $$combine("../", $$needle)
    exists($$needle) {
        return($$system("python $$needle -p"))
    }
    needle = $$combine("../", $$needle)
    exists($$needle) {
        return($$system("python $$needle -p"))
    }

    message("Couldn't find base directory")
    return("")
}

PROJECT_ROOT = $$getBaseDirectory()
PROJECT_ROOT = $$replace(PROJECT_ROOT, "\\\\", "/")

PUBLIC_DIRECTORY = $$join($$list($$PROJECT_ROOT, "public", $$TARGET_SPEC), "/")

PUBLIC_BIN = $$join($$list($$PUBLIC_DIRECTORY, "bin"), "/")
PUBLIC_INC = $$join($$list($$PUBLIC_DIRECTORY, "inc"), "/")
PUBLIC_LIB = $$join($$list($$PUBLIC_DIRECTORY, "lib"), "/")

DESTDIR = $$PUBLIC_BIN

INCLUDEPATH += $$PUBLIC_INC

LIBS += $$combine(-L, $$PUBLIC_LIB)
LIBS += $$combine(-L, $$PUBLIC_BIN)

CONFIG += c++17
CONFIG += console

QMAKE_CXXFLAGS += -Wall
QMAKE_CXXFLAGS += -Werror
QMAKE_CXXFLAGS += -Wextra

win32: DEFINES += WIN32
unix:  DEFINES += LINUX

createDirectory($$PUBLIC_BIN)
createDirectory($$PUBLIC_INC)
createDirectory($$PUBLIC_LIB)