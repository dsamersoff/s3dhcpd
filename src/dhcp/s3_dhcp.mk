OBJS_DHCP=$(patsubst $(SRC)/src/dhcp/%.cxx,obj/%.o,$(wildcard $(SRC)/src/dhcp/*.cxx))
INCLUDE_DHCP=-I$(SRC)/src/dhcp

obj/dhcp_packet.o: $(SRC)/src/dhcp/dhcp_packet.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/dhcp_net.o: $(SRC)/src/dhcp/dhcp_net.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<
