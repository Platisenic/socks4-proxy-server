CXX=g++
CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system -lboost_regex
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))
SOCKS_SERVER = socks_server
CONSOLE_CGI = hw4.cgi
HTTP_SERVER = http_server

.PHONY: all clean prepare

all: $(SOCKS_SERVER) $(CONSOLE_CGI)

$(SOCKS_SERVER): $(SOCKS_SERVER).cpp socks4.hpp
	$(CXX) $< -o $@ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

$(CONSOLE_CGI): $(CONSOLE_CGI).cpp socks4.hpp htmlmsg.hpp
	$(CXX) $< -o $@ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

$(HTTP_SERVER): $(HTTP_SERVER).cpp 
	$(CXX) $< -o $@ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

clean:
	rm -rf $(SOCKS_SERVER) $(CONSOLE_CGI) $(HTTP_SERVER) working_dir

run_socks_server: $(SOCKS_SERVER)
	./$(SOCKS_SERVER) 3333

run_http_server: $(HTTP_SERVER) $(CONSOLE_CGI)
	./$(HTTP_SERVER) 3600

run_console_cgi: $(CONSOLE_CGI)
	export QUERY_STRING="h0=140.113.213.44&p0=5556&f0=t1.txt&sh=140.113.213.44&sp=3333" && ./$(CONSOLE_CGI)

run_np_single:
	rm -rf working_dir
	mkdir working_dir
	cp test.html working_dir/test.html
	cp -r bin working_dir/bin
	cd working_dir && ./../np_single_golden 5556

prepare:
	mkdir -p bin
	cp `which ls` bin/ls
	cp `which cat` bin/cat
	g++ commands/noop.cpp -o bin/noop
	g++ commands/number.cpp -o bin/number
	g++ commands/removetag0.cpp -o bin/removetag0
	g++ commands/removetag.cpp -o bin/removetag
	g++ commands/delayedremovetag.cpp -o bin/delayedremovetag
