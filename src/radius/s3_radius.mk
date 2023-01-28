OBJS_RADIUS=$(patsubst $(SRC)/src/radius/%.cxx,obj/%.o,$(wildcard $(SRC)/src/radius/*.cxx))
INCLUDE_RADIUS=-I$(SRC)/src/radius

obj/radius_packet.o: $(SRC)/src/radius/radius_packet.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/radius_net.o: $(SRC)/src/radius/radius_net.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<
