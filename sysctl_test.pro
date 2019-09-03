# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

#TARGET = sysfs_test

LINUX_HEADERS_PATH = /usr/src/linux-headers-$$system(uname -r)
ARCH = x86

HEADERS =

SOURCES = \
   $$PWD/sysctl_test.c

INCLUDEPATH += $$system(find -L $$LINUX_HEADERS_PATH/include -type d)
INCLUDEPATH += $$system(find -L $$LINUX_HEADERS_PATH/arch/$$ARCH/include -type d)

#DEFINES = 

