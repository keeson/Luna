# Project: mpi-featselect-v2

CPPFLAGS = -O2
LDFLAGS = -Xlinker "-(" -lpthread -lcrypto -lz -lm -lalgo_common -llog4cpp -lrt -Xlinker "-)" -L/home/a/lib64  
INCLUDES = -I/home/a/share/algo1/openmpi-current/include -I/home/a/include -I/home/a/include/log4cpp/ -I/home/a/include/algo_common/ -I./include


.PHONY:all
all: mpiFeatSel
	cp mpiFeatSel ../../../../mpi-featsel-v2/bin/

%.o : %.cpp
	 mpic++ $(CPPFLAGS) -c $< -o $@ $(INCLUDES)

mpiFeatSel: main.o inslookup.o featureSel.o OWLQN.o TerminationCriterion.o
	mpic++ $^ $(LDFLAGS) -o $@  $(INCLUDES)
	
.PHONY:clean
clean:
	rm -fr *.o
	rm -f mpiFeatSel ../../../../mpi-featsel-v2/bin/mpiFeatSel