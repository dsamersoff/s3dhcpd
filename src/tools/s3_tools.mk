OBJS_TOOLS=$(patsubst $(SRC)/src/tools/%.cxx,obj/%.o,$(wildcard $(SRC)/src/tools/*.cxx))
OBJS_TOOLS+=$(patsubst $(SRC)/src/tools/%.c,obj/%.o,$(wildcard $(SRC)/src/tools/*.c))
INCLUDE_TOOLS=-I$(SRC)/src/tools

obj/subnets.o: $(SRC)/src/tools/subnets.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/daemon.o: $(SRC)/src/tools/daemon.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/md5.o: $(SRC)/src/tools/md5.c
	$(CC) -c $(CFLAGS) -o $@ $<
