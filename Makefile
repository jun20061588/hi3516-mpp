AR=arm-himix200-linux-ar
CC=arm-himix200-linux-gcc
CC_FLAGS=-mcpu=arm926ej-s \
		-mno-unaligned-access \
		-fno-aggressive-loop-optimizations \
		-ffunction-sections \
		-fdata-sections

DEFINES=-Dhi3516dv300 \
		-DSENSOR0_TYPE=SONY_IMX327_MIPI_2M_30FPS_12BIT \
		-DSENSOR1_TYPE=SONY_IMX327_MIPI_2M_30FPS_12BIT \
		-DHI_RELEASE -DHI_XXXX -DISP_V2 -DHI_ACODEC_TYPE_INNER

INCLUDES=-I./include/

LOCAL_LIBS=./lib/libmpi.a \
	./lib/lib_hiae.a \
	./lib/libisp.a \
	./lib/lib_hiawb.a \
	./lib/libhi_cipher.a \
	./lib/libVoiceEngine.a \
	./lib/libupvqe.a \
	./lib/libdnvqe.a \
	./lib/libive.a \
	./lib/libmd.a \
	./lib/libsecurec.a \
	./lib/lib_hidrc.a \
	./lib/lib_hildci.a \
	./lib/lib_hidehaze.a \
	./lib/libhdmi.a \
	./lib/libsns_imx327.a \
	./lib/libsns_imx327_2l.a

USER_LIBS=-lpthread -lm -ldl

LIBS=$(LOCAL_LIBS) $(USER_LIBS)

MODULE=himpp
#SOURCES = $(wildcard *.c *.cpp)
#OBJECTS:= $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

OBJECTS:=audio_dl_adp.o \
		audio_aac_adp.o \
		sample_comm_audio.o \
		sample_comm_isp.o \
		sample_comm_sys.o \
		sample_comm_venc.o \
		sample_comm_vi.o \
		sample_comm_vo.o \
		sample_comm_vpss.o \
		sample_audio.o \
		sample_aenc.o \
		sample_venc.o
		
%.o: %.c
	$(CC) -g -c $< -o $@ $(DEFINES) $(CC_FLAGS) $(INCLUDES)

static: $(OBJECTS)
	$(AR) rcsv lib$(MODULE).a $(OBJECTS)

	$(CC) -static -o $(MODULE) main.c lib$(MODULE).a  $(INCLUDES) $(LIBS)

dynamic: $(OBJECTS)
	$(CC) -shared -fPIC -o lib$(MODULE).so $(OBJECTS)

	$(CC) -o $(MODULE)_g main.c lib$(MODULE).so  $(INCLUDES) $(LIBS)

all: static install


LIVEMEDIA_DIR=../live555/liveMedia

install:
	-rm -rf $(LIVEMEDIA_DIR)/$(MODULE)/include/*
	-rm -rf $(LIVEMEDIA_DIR)/$(MODULE)/lib/*
	mkdir -p $(LIVEMEDIA_DIR)/$(MODULE)/include/
	mkdir -p $(LIVEMEDIA_DIR)/$(MODULE)/lib/
	cp -fr include/* sample_comm.h $(LIVEMEDIA_DIR)/$(MODULE)/include/
	cp -fr lib/lib* lib$(MODULE).a $(LIVEMEDIA_DIR)/$(MODULE)/lib/

clean:
	rm -rfv *.o lib$(MODULE).a lib$(MODULE).so $(MODULE)
