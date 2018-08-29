CC = g++
CFLAGS = -std=c++11 -Wall -Wextra -Ofast -frename-registers -march=native
ROBUST = -D ROBUST
PROG = upstream_features.cpp
OUT = giscup-2018

fast: $(PROG) read_graph.h json.h
	$(CC) $(CFLAGS) -o $(OUT) $(PROG)

robust: $(PROG) read_graph.h json.h json_fast.h
	$(CC) $(ROBUST) $(CFLAGS) -o $(OUT) $(PROG)

clean:
	rm $(OUT)
