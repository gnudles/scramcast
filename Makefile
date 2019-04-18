CXXFLAGS =	-fPIC -g -Wall -fmessage-length=0  -pthread
CFLAGS =	-fPIC -g -Wall -fmessage-length=0  -pthread
OBJS =		scramcast.o ScramcastMainServer.o ScramcastSubServer.o ScramcastServer.o ScramcastMemory.o gettimeofday.o
$(OBJ): Makefile sc_defines.h

LIBS = -lrt -lm -lpthread

TARGET =	libscramcast.so
TARGET_STATIC =	libscramcast.a
all:	$(TARGET) $(TARGET_STATIC) scramdmn clear_scramcast
$(TARGET_STATIC):	$(OBJS)
	ar rcs $(TARGET_STATIC) $(OBJS)
$(TARGET):	$(OBJS)
	$(CXX) -shared -fPIC -o $(TARGET) $(OBJS) $(LIBS)
scramdmn: scramdmn.o $(TARGET)
	$(CXX) -o $@ scramdmn.o -L. -lscramcast $(LIBS)
clear_scramcast: clear.o $(TARGET)
	$(CXX) -o $@ clear.o -L. -lscramcast $(LIBS)
clean:
	rm -f $(OBJS) $(TARGET) $(TARGET_STATIC)
