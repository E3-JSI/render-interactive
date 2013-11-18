#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/746986994/sqlite3.o \
	${OBJECTDIR}/_ext/746986994/sqlitedb.o \
	${OBJECTDIR}/semisupervisor/nbc.o \
	${OBJECTDIR}/semisupervisor/ss_dataset.o \
	${OBJECTDIR}/semisupervisor/ss_km.o \
	${OBJECTDIR}/semisupervisor/strftrspace.o \
	${OBJECTDIR}/sssrvd/ss_controller.o \
	${OBJECTDIR}/sssrvd/sssrv_main.o \
	${OBJECTDIR}/sssrvd/stdafx.o \
	${OBJECTDIR}/websrv/mong_srv.o \
	${OBJECTDIR}/websrv/mongoose/main.o \
	${OBJECTDIR}/websrv/mongoose/mongoose.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/render-interactive

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/render-interactive: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/render-interactive ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/746986994/sqlite3.o: ../glib/misc/sqlite3.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/746986994
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/746986994/sqlite3.o ../glib/misc/sqlite3.c

${OBJECTDIR}/_ext/746986994/sqlitedb.o: ../glib/misc/sqlitedb.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/746986994
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/746986994/sqlitedb.o ../glib/misc/sqlitedb.cpp

${OBJECTDIR}/semisupervisor/nbc.o: semisupervisor/nbc.cpp 
	${MKDIR} -p ${OBJECTDIR}/semisupervisor
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/semisupervisor/nbc.o semisupervisor/nbc.cpp

${OBJECTDIR}/semisupervisor/ss_dataset.o: semisupervisor/ss_dataset.cpp 
	${MKDIR} -p ${OBJECTDIR}/semisupervisor
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/semisupervisor/ss_dataset.o semisupervisor/ss_dataset.cpp

${OBJECTDIR}/semisupervisor/ss_km.o: semisupervisor/ss_km.cpp 
	${MKDIR} -p ${OBJECTDIR}/semisupervisor
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/semisupervisor/ss_km.o semisupervisor/ss_km.cpp

${OBJECTDIR}/semisupervisor/strftrspace.o: semisupervisor/strftrspace.cpp 
	${MKDIR} -p ${OBJECTDIR}/semisupervisor
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/semisupervisor/strftrspace.o semisupervisor/strftrspace.cpp

${OBJECTDIR}/sssrvd/ss_controller.o: sssrvd/ss_controller.cpp 
	${MKDIR} -p ${OBJECTDIR}/sssrvd
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/sssrvd/ss_controller.o sssrvd/ss_controller.cpp

${OBJECTDIR}/sssrvd/sssrv_main.o: sssrvd/sssrv_main.cpp 
	${MKDIR} -p ${OBJECTDIR}/sssrvd
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/sssrvd/sssrv_main.o sssrvd/sssrv_main.cpp

${OBJECTDIR}/sssrvd/stdafx.o: sssrvd/stdafx.cpp 
	${MKDIR} -p ${OBJECTDIR}/sssrvd
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/sssrvd/stdafx.o sssrvd/stdafx.cpp

${OBJECTDIR}/websrv/mong_srv.o: websrv/mong_srv.cpp 
	${MKDIR} -p ${OBJECTDIR}/websrv
	${RM} $@.d
	$(COMPILE.cc) -g -DOPENBLAS -I../glib/base -I../QMiner/src -I../glib/mine -I../glib/misc -I../glib/concurrent -MMD -MP -MF $@.d -o ${OBJECTDIR}/websrv/mong_srv.o websrv/mong_srv.cpp

${OBJECTDIR}/websrv/mongoose/main.o: websrv/mongoose/main.c 
	${MKDIR} -p ${OBJECTDIR}/websrv/mongoose
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/websrv/mongoose/main.o websrv/mongoose/main.c

${OBJECTDIR}/websrv/mongoose/mongoose.o: websrv/mongoose/mongoose.c 
	${MKDIR} -p ${OBJECTDIR}/websrv/mongoose
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/websrv/mongoose/mongoose.o websrv/mongoose/mongoose.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/render-interactive

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
