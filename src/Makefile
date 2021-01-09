## This is an example Makefile. 
## The KTracker Makefile gets generated automatically from Makefile.in 
## and *.config

#WPTIME=-DGET_WP_TIME
#PAUSED_TIME=-DGET_PAUSED_TIME -DGET_APP_TIME
#PAUSED_TIME=-DGET_APP_TIME
#PAUSED_TIME=-DGET_PAUSED_TIME 
#WP=-DWPROTECT
#NODT=-DNO_DIRTY_TRACKING
#NODT=-DNO_DIRTY_TRACKING_SLEEP
DEFS=$(WP) $(WPTIME) $(PAUSED_TIME) $(NODT)
DEFS+=-DPB_WRITE_DCL
LIBS=-lpthread -lnuma
FLAGS=-g $(DEFS) -O3 
SRC=numalib.c pagemap.c proc_maps.c ptimer.c common.c htable.c

# software tracker (PBSim)
tracker: 
	gcc $(FLAGS) -o tracker tracker.c $(SRC) $(LIBS) 

clean:
	rm -f *.o tracker 
