##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##

OBJS_1 = $(SRCS_1:.c=.o)
$(TARGET_1): $(OBJS_1) $(PREPARE)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) $(LFLAGS) -o $@ $(addprefix .obj/,$(notdir $(OBJS_1))) -Wl,--start-group $(LIBS) $(LIBS_1) -Wl,--end-group
	cp $@ $(OUTPUT_DIR)

$(OBJS_1): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<

OBJS_2 = $(SRCS_2:.c=.o)
$(TARGET_2): $(OBJS_2) $(PREPARE)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) $(LFLAGS) -o $@ $(addprefix .obj/,$(notdir $(OBJS_2))) -Wl,--start-group $(LIBS) $(LIBS_2) -Wl,--end-group
	cp $@ $(OUTPUT_DIR)

$(OBJS_2): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<

OBJS_3 = $(SRCS_3:.c=.o)
$(TARGET_3): $(OBJS_3) $(PREPARE)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) $(LFLAGS) -o $@ $(addprefix .obj/,$(notdir $(OBJS_3))) -Wl,--start-group $(LIBS) $(LIBS_3) -Wl,--end-group
	cp $@ $(OUTPUT_DIR)

$(OBJS_3): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<

OBJS_4 = $(SRCS_4:.c=.o)
$(TARGET_4): $(OBJS_4) $(PREPARE)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) $(LFLAGS) -o $@ $(addprefix .obj/,$(notdir $(OBJS_4))) -Wl,--start-group $(LIBS) $(LIBS_4) -Wl,--end-group
	cp $@ $(OUTPUT_DIR)

$(OBJS_4): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<


OBJS_5 = $(SRCS_5:.c=.o)
$(TARGET_5): $(OBJS_5) $(PREPARE)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) $(LFLAGS) -o $@ $(addprefix .obj/,$(notdir $(OBJS_5))) -Wl,--start-group $(LIBS) $(LIBS_5) -Wl,--end-group
	cp $@ $(OUTPUT_DIR)

$(OBJS_5): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<

OBJS_6 = $(SRCS_6:.c=.o)
$(TARGET_6): $(OBJS_6) $(PREPARE)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) $(LFLAGS) -o $@ $(addprefix .obj/,$(notdir $(OBJS_6))) -Wl,--start-group $(LIBS) $(LIBS_6) -Wl,--end-group
	cp $@ $(OUTPUT_DIR)

$(OBJS_6): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<


OBJS_7 = $(SRCS_7:.c=.o)
$(TARGET_7): $(OBJS_7) $(PREPARE)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) $(LFLAGS) -o $@ $(addprefix .obj/,$(notdir $(OBJS_7))) -Wl,--start-group $(LIBS) $(LIBS_7) -Wl,--end-group
	cp $@ $(OUTPUT_DIR)

$(OBJS_7): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<

OBJS_8 = $(SRCS_8:.c=.o)
$(TARGET_8): $(OBJS_8) $(PREPARE)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) $(LFLAGS) -o $@ $(addprefix .obj/,$(notdir $(OBJS_8))) -Wl,--start-group $(LIBS) $(LIBS_8) -Wl,--end-group
	cp $@ $(OUTPUT_DIR)

$(OBJS_8): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<

