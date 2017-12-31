include ../Makefile.param

HIARCH?=hi3518
LIBC?=uclibc
HIDBG?=HI_DEBUG
COUURENT_PATH = $(shell pwd)
INCLUDE_DIR := $(COUURENT_PATH)/include
OUTPUT_DIR = $(shell pwd)/output

CFLAGS += -I $(INCLUDE_DIR) 
CFLAGS += -I $(REL_LIB)/include
CFLAGS += -I $(PROJECT_DIR)/include
CFLAGS += -I $(COMMON_DIR)
LDFLAGS := -L $(REL_LIB)

#CFILE_DIR := $(COUURENT_PATH)/common
LD_SHARE = -L ./lib
LD =  $(CC)ld
TAG := $(COUURENT_PATH)/test_demo
LIBS = -lpthread -lrt -lm
export CFLAGS
export OUTPUT_DIR
export CC
export COMMON_DIR
#将CFILE_DIR目录下的所有点c文件名(包括路径)放到SRC中
#SRC  :=  $(wildcard $(CFILE_DIR)/*.c)

#将当前目录下的所有点c文件名追加到SRC中
SRC  :=  $(wildcard $(COUURENT_PATH)/*.c)
#去掉路径只留文件名存到SRC_FILE中
SRC_FILE:= $(notdir $(SRC))

#将SRC包含的所有点c文件 转换成相应的点o文件 再存放到OBJS中
OBJS := $(SRC:%.c=%.o)
#将SRC_FILE包含的所有点c文件 转换成相应的点o文件 再存放到OBJS_FILE中
OBJS_FILE := $(SRC_FILE:%.c=%.o)
export LD_LIBRARY_PATH = ./lib
#当要匹配点c文件时 到后缀的两个目录下找
#vpath %.c $(CFILE_DIR) $(COUURENT_PATH)



SUB_DIRS :=   $(COUURENT_PATH)/common		\
	      $(COUURENT_PATH)/sysConfig	\
	      $(COUURENT_PATH)/localNetInfo	\
	      $(COUURENT_PATH)/talkCommandTranslate \
	      $(COUURENT_PATH)/cellInformation  \
	      $(COUURENT_PATH)/talkAudio	\
	      $(COUURENT_PATH)/wavPlay	 \
	      $(COUURENT_PATH)/keyBoard \
	      $(COUURENT_PATH)/tftpUpdateServer \
	      $(COUURENT_PATH)/watchdog  \
	      $(COUURENT_PATH)/security
		 
		   
#
define make_subdir
 @for subdir in $(SUB_DIRS) ; do \
  ( cd $$subdir && make ) \
 done;
endef
#
define clean_subdir
 @for subdir in $(SUB_DIRS) ; do \
  ( cd $$subdir && make clean ) \
 done;
endef
#
MPI_LIBS += $(REL_LIB)/libive.so
MPI_LIBS += $(REL_LIB)/libisp.so
MPI_LIBS += $(REL_LIB)/lib_hiawb.so
MPI_LIBS += $(REL_LIB)/libsns_imx222.so
MPI_LIBS += $(REL_LIB)/lib_hidefog.so
MPI_LIBS += $(REL_LIB)/lib_hiaf.so
MPI_LIBS += $(REL_LIB)/lib_hiae.so
VQE_LIBS += $(COUURENT_PATH)/lib/libUsCam.so
VQE_LIBS += $(COUURENT_PATH)/lib/libVqe.so
VQE_LIBS += $(REL_LIB)/libmpi.so 
GEN_O = $(OUTPUT_DIR)/*.o
$(TAG):  subobjs $(OBJS_FILE) $(COMM_OBJ)
	$(CC) $(CFLAGS)   -o  $@ $(LDFLAGS) $(OBJS_FILE)  $(GEN_O)  $(COMM_OBJ)  $(LIBS)  $(AUDIO_LIBA)  $(MPI_LIBS)   $(VQE_LIBS) $(LD_SHARE)
	cp $(TAG)  /rootnfs/
subobjs:
	echo "subobjs"
	$(call make_subdir)
	
$(OUTPUT_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c  $< -o  $@
	
clean:
	rm -rf *.o $(TAG) $(GEN_O) $(COMM_OBJ)
















