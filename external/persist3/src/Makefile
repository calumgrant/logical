OBJS = cmdline.o 
LIST_OBJS = shared_list.o
POBJS = persist.o persist_unix.o
BENCH_OBJS = bench.o

CPPFLAGS = -I. -I../include -Wall -O2

all : libpersist.a cmdline lists bench

libpersist.a : $(POBJS)
	ar rs libpersist.a $(POBJS)

cmdline : $(OBJS) libpersist.a
	g++ -L. $(OBJS) -lpersist -o cmdline

lists: $(LIST_OBJS) libpersist.a
	g++ -L. $(LIST_OBJS) -lpersist -o lists

bench: $(BENCH_OBJS) libpersist.a
	g++ -L. $(BENCH_OBJS) -lpersist -o bench -lmysqlclient

install:
	cp libpersist.a /usr/local/lib
	cp persist.h persist_unix.h /usr/local/include

clean:
	-rm *.map *.a *.o cmdline bench lists

distclean: clean
	-rm *.map *.a *.o cmdline bench lists
	-rm -r ../vc6/Debug
	-rm -r ../vc6/Release
	-rm ../vc6/vc6.opt ../vc6/vc6.ncb ../vc6/vc6.plg
	-rm ../vc7/vc7.ncb ../vc7/vc7.suo
	-rm -r ../vc7/*/Debug
	-rm -r ../vc7/*/Release
	-rm ../vc7/*/*.map
	
tarball: distclean
	cd ../.. ; tar czf persist-0.9.tgz persist-0.9 ; zip -ur persist-0.9.zip persist-0.9


