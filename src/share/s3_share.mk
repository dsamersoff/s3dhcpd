OBJS_SHARE=$(patsubst $(SRC)/src/share/%.cxx,obj/%.o,$(wildcard $(SRC)/src/share/*.cxx))
INCLUDE_SHARE=-I$(SRC)/src/share

obj/dsStrstream.o: $(SRC)/src/share/dsStrstream.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/dsForm.o: $(SRC)/src/share/dsForm.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/dsHashTable.o: $(SRC)/src/share/dsHashTable.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/dsSmartException.o: $(SRC)/src/share/dsSmartException.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/dsSlightFunctions.o: $(SRC)/src/share/dsSlightFunctions.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/dsApprc.o: $(SRC)/src/share/dsApprc.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/dsGetopt.o: $(SRC)/src/share/dsGetopt.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/dsLog.o: $(SRC)/src/share/dsLog.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<
