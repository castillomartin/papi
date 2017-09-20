# Author: Philip Mucci, University of Tennessee 
# Mods: 

CC:=gcc
CFLAGS?=-O1 -g -Wall
MY_RPATH=-Wl,-rpath -Wl,$(PWD)
MONITOR_INC_PATH=$(PWD)/monitor/install/include
MONITOR_LIB_PATH=$(PWD)/monitor/install/lib
MONITOR_RPATH=-Wl,-rpath -Wl,$(MONITOR_LIB_PATH)

OPENMP_TESTS=omp omp2 omp-d omp2-d
PTHREAD_TESTS=pthreads pthreads2 pthreads3 pthreads4 pthreads5 pthreads6 pthreads-d pthreads2-d pthreads3-d pthreads4-d pthreads5-d pthreads6-d 

TESTS=$(OPENMP_TESTS) $(PTHREAD_TESTS)
LIBS=libhl.so libdummy.so

all: monitor/install $(LIBS) $(TESTS)

monitor/install: monitor/Makefile 
	$(MAKE) -C monitor install

monitor/Makefile:
	cd monitor && ./configure --prefix=$$PWD/install

libhl.so: region.c monitor_callbacks.c
	$(CC) -I$(MONITOR_INC_PATH) -shared -pthread $(CFLAGS) region.c monitor_callbacks.c -o $@

libdummy.so: region_dummy.c
	$(CC) -shared -pthread $(CFLAGS) region_dummy.c -o $@

#
# OpenMP
#

omp: tests/omp.c libdummy.so
	$(CC) $(CFLAGS) -fopenmp $< -o $@ -L. -ldummy $(MY_RPATH)
omp2: tests/omp2.c libdummy.so
	$(CC) $(CFLAGS) -fopenmp $< -o $@ -L. -ldummy $(MY_RPATH)
omp-d: tests/omp.c libdummy.so
	$(CC) $(CFLAGS) -fopenmp $< -o $@ -L. -lhl $(MY_RPATH) -L$(MONITOR_LIB_PATH) -lmonitor $(MONITOR_RPATH)
omp2-d: tests/omp2.c libdummy.so
	$(CC) $(CFLAGS) -fopenmp $< -o $@ -L. -lhl $(MY_RPATH) -L$(MONITOR_LIB_PATH) -lmonitor $(MONITOR_RPATH)

#
# Pthreads
#

pthreads: tests/pthreads.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -ldummy $(MY_RPATH)
pthreads2: tests/pthreads2.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -ldummy $(MY_RPATH)
pthreads3: tests/pthreads3.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -ldummy $(MY_RPATH)
pthreads4: tests/pthreads4.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -ldummy $(MY_RPATH)
pthreads5: tests/pthreads5.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -ldummy $(MY_RPATH)
pthreads6: tests/pthreads6.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -ldummy $(MY_RPATH)
pthreads-d: tests/pthreads.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -lhl $(MY_RPATH) -L$(MONITOR_LIB_PATH) -lmonitor $(MONITOR_RPATH)
pthreads2-d: tests/pthreads2.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -lhl $(MY_RPATH) -L$(MONITOR_LIB_PATH) -lmonitor $(MONITOR_RPATH)
pthreads3-d: tests/pthreads3.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -lhl $(MY_RPATH) -L$(MONITOR_LIB_PATH) -lmonitor $(MONITOR_RPATH)
pthreads4-d: tests/pthreads4.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -lhl $(MY_RPATH) -L$(MONITOR_LIB_PATH) -lmonitor $(MONITOR_RPATH)
pthreads5-d: tests/pthreads5.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -lhl $(MY_RPATH) -L$(MONITOR_LIB_PATH) -lmonitor $(MONITOR_RPATH)
pthreads6-d: tests/pthreads6.c libdummy.so
	$(CC) $(CFLAGS) -pthread $< -o $@ -L. -lhl $(MY_RPATH) -L$(MONITOR_LIB_PATH) -lmonitor $(MONITOR_RPATH)

clean distclean:
	rm -f $(LIBS) $(TESTS) *~ core *.o a.out *~ */*~
	$(MAKE) -C monitor distclean

#
# Test targets
#

test: test-openmp test-pthreads
test-openmp: test-openmp-preload test-openmp-direct
test-openmp-preload: test-omp1 test-omp2 test-omp3 test-omp4
test-openmp-direct: test-omp1-d test-omp2-d test-omp3-d test-omp4-d
test-pthreads: test-pthreads-preload test-pthreads-direct
test-pthreads-preload: test-pthreads1 test-pthreads2 test-pthreads3 test-pthreads4 test-pthreads5 test-pthreads6
test-pthreads-direct: test-pthreads1-d test-pthreads2-d test-pthreads3-d test-pthreads4-d test-pthreads5-d test-pthreads6-d

#
# OpenMP tests
#

test-omp1:
	./monitor/install/bin/monitor-run -i ./libhl.so ./omp
test-omp2:
	./monitor/install/bin/monitor-run -i ./libhl.so ./omp2
test-omp3:
	OMP_NUM_THREADS=16 ./monitor/install/bin/monitor-run -i ./libhl.so ./omp
test-omp4:
	OMP_NUM_THREADS=16 ./monitor/install/bin/monitor-run -i ./libhl.so ./omp2
test-omp1-d:
	./omp-d
test-omp2-d:
	./omp2-d
test-omp3-d:
	OMP_NUM_THREADS=16 ./omp-d
test-omp4-d:
	OMP_NUM_THREADS=16 ./omp2-d

#
# Pthreads tests
# 

test-pthreads1:
	./monitor/install/bin/monitor-run -i ./libhl.so ./pthreads
test-pthreads2:
	./monitor/install/bin/monitor-run -i ./libhl.so ./pthreads2
test-pthreads3:
	./monitor/install/bin/monitor-run -i ./libhl.so ./pthreads3
test-pthreads4:
	./monitor/install/bin/monitor-run -i ./libhl.so ./pthreads4
test-pthreads5:
	./monitor/install/bin/monitor-run -i ./libhl.so ./pthreads5
test-pthreads6:
	./monitor/install/bin/monitor-run -i ./libhl.so ./pthreads6
test-pthreads1-d:
	./pthreads-d
test-pthreads2-d:
	./pthreads2-d
test-pthreads3-d:
	./pthreads3-d
test-pthreads4-d:
	./pthreads4-d
test-pthreads5-d:
	./pthreads5-d
test-pthreads6-d:
	./pthreads6-d
