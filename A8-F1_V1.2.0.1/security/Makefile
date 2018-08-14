
LOCAL_SRC_FILES := $(wildcard *.c)

OBJS := $(patsubst %.c, $(OUTPUT_DIR)/%.o, $(LOCAL_SRC_FILES))

all : $(OBJS)

$(OUTPUT_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c  $< -o  $@

clean:
	rm -rf *.o

