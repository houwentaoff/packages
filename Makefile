SUBDIRS = data_transfer

default:all

ifneq ($(PC), 1)
include env.mk
endif

all:
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d all; done

clean:
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d clean; done
	$(RM) *~ *.o *.a

.PHONY:clean

