OBJS_PF=$(patsubst $(SRC)/src/pf/%.cxx,obj/%.o,$(wildcard $(SRC)/src/pf/*.cxx))
INCLUDE_PF=-I$(SRC)/src/pf

obj/pf.o: $(SRC)/src/pf/pf.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<
