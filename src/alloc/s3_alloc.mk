OBJS_ALLOC=$(patsubst $(SRC)/src/alloc/%.cxx,obj/%.o,$(wildcard $(SRC)/src/alloc/*.cxx))
INCLUDE_ALLOC=-I$(SRC)/src/alloc

obj/ipallocator.o: $(SRC)/src/alloc/ipallocator.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/ipallocator_sql.o: $(SRC)/src/alloc/ipallocator_sql.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<
