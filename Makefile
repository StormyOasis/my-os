#CC = gcc
#	Linux C++ compiler
#CPPC = g++
#	Windows C++ compiler
CC = gcc
CPPC = g++
#DJGPP needs ld-elf
#LD = ld-elf
LD = ld
ASM = nasm
ASMFLAGS = -f aout
#ASMFLAGS = -f elf32

ASMPATH = src/asm
OBJPATH = obj
SOURCEPATH = src
INCLUDEPATH = include
BINARYPATH = bin

CFLAGS =  -nostdlib  -nostartfiles -nodefaultlibs -ffreestanding -fno-exceptions
CPPFLAGS = -nostdinc++ -fno-rtti -nostdlib  -nostartfiles -nodefaultlibs -fno-exceptions
#-save-temps

#LDFLAGS = --script=ldscript --oformat binary -s -Bstatic
LDFLAGS = --script=ldscript -g --oformat elf32-i386

OBJS = $(OBJPATH)/initmain.o $(OBJPATH)/kern_mem.c.o $(OBJPATH)/main.c.o $(OBJPATH)/cpp.c.o $(OBJPATH)/ipc.c.o $(OBJPATH)/ipc.cpp.o $(OBJPATH)/kernel.c.o $(OBJPATH)/sysman.c.o $(OBJPATH)/util.c.o $(OBJPATH)/sched.c.o $(OBJPATH)/tss.c.o $(OBJPATH)/mutex.c.o $(OBJPATH)/badt.c.o $(OBJPATH)/proc.c.o $(OBJPATH)/cgstubs.c.o $(OBJPATH)/badt.cpp.o

PROG = $(BINARYPATH)/initmain.run
STAGE1 = $(BINARYPATH)/stage1.run
STAGE2 = $(BINARYPATH)/stage2.run

# top-level rule, to compile kernel(not loaders).
all: $(PROG)

# rule to link the program
$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(PROG)
        
$(OBJPATH)/main.c.o: $(SOURCEPATH)/main.c $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h $(INCLUDEPATH)/types.h $(INCLUDEPATH)/defs.h $(INCLUDEPATH)/util.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/main.c -o $(OBJPATH)/main.c.o
	
$(OBJPATH)/sysman.c.o: $(SOURCEPATH)/sysman.c $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/sysman.c -o $(OBJPATH)/sysman.c.o
	
$(OBJPATH)/util.c.o: $(SOURCEPATH)/util.c $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/util.c -o $(OBJPATH)/util.c.o
	
$(OBJPATH)/proc.c.o: $(SOURCEPATH)/proc.c $(INCLUDEPATH)/proc.h $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h $(INCLUDEPATH)/sched.h $(INCLUDEPATH)/kernel.h $(INCLUDEPATH)/tss.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/proc.c -o $(OBJPATH)/proc.c.o	
	
$(OBJPATH)/ipc.c.o: $(SOURCEPATH)/ipc.c $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/ipc.c -o $(OBJPATH)/ipc.c.o
	
$(OBJPATH)/ipc.cpp.o: $(SOURCEPATH)/ipc.cpp $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h
	$(CPPC) $(CPPFLAGS) -c $(SOURCEPATH)/ipc.cpp -o $(OBJPATH)/ipc.cpp.o	
	
$(OBJPATH)/sched.c.o: $(SOURCEPATH)/sched.cpp $(INCLUDEPATH)/sched.h $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h $(INCLUDEPATH)/cpp.h $(SOURCEPATH)/cpp.cpp
	$(CPPC) $(CPPFLAGS) -c $(SOURCEPATH)/sched.cpp -o $(OBJPATH)/sched.c.o
	
$(OBJPATH)/kernel.c.o: $(SOURCEPATH)/kernel.c $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h $(INCLUDEPATH)/kernel.h $(INCLUDEPATH)/defs.h $(INCLUDEPATH)/sched.h $(INCLUDEPATH)/tss.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/kernel.c -o $(OBJPATH)/kernel.c.o
	
$(OBJPATH)/kern_mem.c.o: $(SOURCEPATH)/kern_mem.c $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/kern_mem.c -o $(OBJPATH)/kern_mem.c.o
	
$(OBJPATH)/tss.c.o: $(SOURCEPATH)/tss.c $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/tss.c -o $(OBJPATH)/tss.c.o
	
$(OBJPATH)/badt.c.o: $(SOURCEPATH)/basicadt.c $(INCLUDEPATH)/basicadt.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/basicadt.c -o $(OBJPATH)/badt.c.o	
	
$(OBJPATH)/badt.cpp.o: $(SOURCEPATH)/basicadt.cpp $(INCLUDEPATH)/badtcpp.h $(INCLUDEPATH)/basicadt.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h
	$(CPPC) $(CPPFLAGS) -c $(SOURCEPATH)/basicadt.cpp -o $(OBJPATH)/badt.cpp.o		
	
$(OBJPATH)/mutex.c.o: $(SOURCEPATH)/mutex.cpp $(INCLUDEPATH)/mutex.h $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h $(INCLUDEPATH)/types.h $(INCLUDEPATH)/defs.h $(INCLUDEPATH)/util.h $(INCLUDEPATH)/tss.h
	$(CPPC) $(CPPFLAGS) -c $(SOURCEPATH)/mutex.cpp -o $(OBJPATH)/mutex.c.o	
	
$(OBJPATH)/cpp.c.o: $(SOURCEPATH)/cpp.cpp $(INCLUDEPATH)/cpp.h $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h $(INCLUDEPATH)/types.h $(INCLUDEPATH)/defs.h $(INCLUDEPATH)/util.h $(INCLUDEPATH)/tss.h
	$(CPPC) $(CPPFLAGS) -c $(SOURCEPATH)/cpp.cpp -o $(OBJPATH)/cpp.c.o		
	
$(OBJPATH)/cgstubs.c.o: $(SOURCEPATH)/cgstubs.c $(INCLUDEPATH)/cgstubs.h $(INCLUDEPATH)/kern_mem.h $(INCLUDEPATH)/ipc.h $(INCLUDEPATH)/sysman.h $(INCLUDEPATH)/types.h $(INCLUDEPATH)/defs.h $(INCLUDEPATH)/util.h $(INCLUDEPATH)/tss.h
	$(CC) $(CFLAGS) -c $(SOURCEPATH)/cgstubs.c -o $(OBJPATH)/cgstubs.c.o	

$(OBJPATH)/initmain.o: $(ASMPATH)/initmain.asm $(ASMPATH)/tss.asm $(ASMPATH)/gdt.asm $(ASMPATH)/idt.asm $(ASMPATH)/video.asm $(ASMPATH)/kernel.asm $(ASMPATH)/timer.asm $(ASMPATH)/lomem.asm $(ASMPATH)/keyboard.asm
	$(ASM) $(ASMFLAGS) -o $(OBJPATH)/initmain.o $(ASMPATH)/initmain.asm
				
		
stage1: $(STAGE1)
$(STAGE1): $(ASMPATH)/stage1.asm
	$(ASM) -f bin $(ASMPATH)/stage1.asm -o $(STAGE1)
	
stage2: $(STAGE2)
$(STAGE2): $(ASMPATH)/stage2.asm
	$(ASM) -f bin $(ASMPATH)/stage2.asm -o $(STAGE2)	
		
full:
	make stage1
	make stage2
	make
		
	objdump  --line-numbers --source $(PROG) >$(OBJPATH)/krnl.lst
	nm --line-numbers  $(PROG) | sort >$(OBJPATH)/krnl.sym
	objcopy -O binary  $(PROG) $(PROG)
				
install:
	make clean
	make full
	cat $(BINARYPATH)/stage1.run  $(BINARYPATH)/stage2.run $(BINARYPATH)/initmain.run > $(BINARYPATH)/boot
#	cat $(BINARYPATH)/boot > /dev/fd0	
	cat $(BINARYPATH)/boot > floppy.img	
	make run
	
run:
	bochs -qf bochsrc.txt 
		#'boot:a' 'floppya: 1_44=floppy.img, status=inserted'

clean:
#	Linux delete
#	rm -f $(OBJPATH)/*.o	
#	rm -f $(BINARYPATH)/*.run
#	rm -f $(BINARYPATH)/boot

#	Windows delete(this does not seem to accept the /Q param??!?!
#	del /Q $(OBJPATH)	
#	del /Q $(BINARYPATH)

