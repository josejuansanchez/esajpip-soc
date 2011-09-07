DEBUG=yes
PLATFORM=Linux
#PLATFORM=Solaris

CXX=g++
SHELL=/bin/bash

OBJS += data/serialize              \
		data/file                   \
		data/file_segment           \
		data/vint_vector

OBJS +=	jpeg2000/point              \
		jpeg2000/coding_parameters  \
		jpeg2000/index_manager      \
		jpeg2000/image_index        \
		jpeg2000/packet_index       \
		jpeg2000/codestream_index   \
		jpeg2000/image_info         \
		jpeg2000/file_manager       \
		jpeg2000/packet				\
		jpeg2000/place_holder		\
		jpeg2000/meta_data			\
		jpeg2000/range

OBJS += ipc/ipc_object              \
		ipc/event                   \
		ipc/mutex                   \
		ipc/rdwr_lock

OBJS += http/protocol               \
		http/response               \
		http/header                 \
		http/request

OBJS += net/socket					\
		net/socket_stream			\
		net/poll_table

OBJS += jpip/jpip                   \
		jpip/woi                    \
		jpip/request                \
		jpip/woi_composer           \
		jpip/databin_writer			\
		jpip/cache_model			\
		jpip/databin_server

OBJS += base                        \
		trace                       \
		app_info					\
		app_config					\
		args_parser					\
		client_info					\
		client_manager					

ifeq ($(DEBUG),yes)							
FLAGS = -g -Wall -fmessage-length=0 -I src
else
FLAGS = -O2 -Wall -fmessage-length=0 -I src -DNDEBUG
endif

LIBS = -lpthread -lm -lrt -lconfig++ -llog4cpp -lproc

ifeq ($(PLATFORM),Solaris)
LIBS += -lnsl -lsocket
FLAGS += -D_NO_DIRENT -D_NO_READPROC
FLAGS += -D_USE_BOOST -D_NO_FAST_FILE
endif

MAIN = packet_information				\
	   basic_server						\
	   esa_jpip_server			 		

obj/%.o: src/%.cc src/%.h
	mkdir -p obj/$$(dirname $*.cc)
	$(CXX) $(FLAGS) -c src/$*.cc -o obj/$*.o 
	
bin/esa_jpip_server: src/esa_jpip_server.cc $(OBJS:%=obj/%.o) version
	mkdir -p bin
	$(CXX) $(FLAGS) src/esa_jpip_server.cc -o $@ $(OBJS:%=obj/%.o) $(LIBS)

bin/basic_server: src/esa_jpip_server.cc $(OBJS:%=obj/%.o) version
	mkdir -p bin
	$(CXX) $(FLAGS) -DBASIC_SERVER src/esa_jpip_server.cc -o $@ $(OBJS:%=obj/%.o) $(LIBS)

bin/packet_information: src/packet_information.cc $(OBJS:%=obj/%.o)
	mkdir -p bin
	$(CXX) $(FLAGS) src/packet_information.cc -o $@ $(OBJS:%=obj/%.o) $(LIBS)

version: VERSION
	(n=$$(cat VERSION); echo "#define VERSION \"$$n\""> src/version.h)
	
doc: documentation

documentation:
	make -C doc

all: $(MAIN:%=bin/%)

clean:
	rm -rf obj bin log/* src/version.h
	make -C doc clean
