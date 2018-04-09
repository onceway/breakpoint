LIB = libdownloadfile.a
LIBPATH = lib
SRCPATH = src
DEMOPATH = demo
DEMO = $(DEMOPATH)/demo
SRCS = $(wildcard $(SRCPATH)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))

all: lib demo

demo: $(DEMO)

$(DEMO): $(DEMOPATH)/demo.cpp
	g++ $^ -o $@ -I$(SRCPATH) -L$(LIBPATH) -lcurl -ldownloadfile -g

lib: $(LIB)

$(LIB) : $(OBJS)
	ar cr $@ $(OBJS)
	rm -f $(OBJS)
	mv $(LIB) $(LIBPATH)

%.o: %.cpp
	g++ -g -c $< -o $@

clean:
	rm -rf $(DEMOPATH)/demo $(LIBPATH)/$(LIB)
