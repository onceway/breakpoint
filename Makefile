LIB = libdownloadfile.a
LIBPATH = lib
SRCPATH = src
OBJS = downloadfile.o

ALL: demo

demo: demo.cpp $(LIB)
	g++ $^ -o $(LIBPATH)/$@ -I. -L$LIBPATH -lcurl -ldownloadfile -g

lib: $(LIB)

$(LIB) : $(OBJS)
	ar cr $@ $(OBJS)
	rm -f $(OBJS)

%.o: %.cpp
	g++ -g -c $< -o $@

clean:
	rm -rf $(LIBPATH)/demo
