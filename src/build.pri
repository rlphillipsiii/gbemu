CONFIG (release, debug|release) {
    TARGET_SPEC = release

    DEFINES += NDEBUG
    DEFINES += RELEASE

    QMAKE_CXXFLAGS += -O3
    QMAKE_LFLAGS += -O3
}

CONFIG (symbols) {
    QMAKE_CXXFLAGS += -g
}

CONFIG (debug, debug|release) {
    TARGET_SPEC = debug

    DEFINES += DEBUG

    QMAKE_CXXFLAGS += -g
    QMAKE_CXXFLAGS += -Og
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

defineTest(publishFiles) {
    fileslist = $$ARGS

    for (target, fileslist) {
        exists($$target) {
            dest = $$join($$list($$PUBLIC_BIN, $$target), "/")
            cmd  = $$quote($$CONFIG_CMD -l $$PWD $$target $$dest)
            system($$cmd)
        }
    }
}

defineTest(publishHeaders) {
    fileslist = $$ARGS

    for (target, fileslist) {
        exists($$target) {
            dest = $$join($$list($$PUBLIC_INC, $$target), "/")
            cmd  = $$quote($$CONFIG_CMD -l $$PWD $$target $$dest)
            system($$cmd)
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

CONFIG_SCRIPT = $$join($$list($$PROJECT_ROOT, "config.py"), "/")

CONFIG_CMD = $$join($$list("python", $$CONFIG_SCRIPT), " ")

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