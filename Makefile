CC = g++
OUTPATH = out
SRCPATH = src

parser: my_time.o pcap_parser.o moex_schemes.o
	$(CC) $(SRCPATH)/parser.cpp -o $(OUTPATH)/parser $(SRCPATH)/my_time.o $(SRCPATH)/pcap_parser.o $(SRCPATH)/moex_schemes.o
	
my_time.o: $(SRCPATH)/my_time.cpp $(SRCPATH)/my_time.h
	$(CC) -c $(SRCPATH)/my_time.cpp -o $(SRCPATH)/$@
	
pcap_parser.o: $(SRCPATH)/pcap_parser.cpp $(SRCPATH)/pcap_parser.h
	$(CC) -c $(SRCPATH)/pcap_parser.cpp -o $(SRCPATH)/$@
	
moex_schemes.o: $(SRCPATH)/moex_schemes.cpp $(SRCPATH)/moex_schemes.h pcap_parser.o my_time.o
	$(CC) -c $(SRCPATH)/moex_schemes.cpp -o $(SRCPATH)/$@
