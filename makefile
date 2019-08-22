all: mix mixal
COMMON_C = mix_float.c mix_types.c
COMMON_H = mix_types.h mix_float.h
COMMON_O = mix_float.o mix_types.o

CPU_C = mix_cpu.c mix_decode.c mix_io.c atomic_queue.c
CPU_H = mix_decode.h mix_io.h atomic_queue.h
CPU_O = mix_cpu.o mix_decode.o mix_io.o atomic_queue.o

ASM_C = mixal.c mix_flotio.c lexparse.c mix_encode.c
ASM_H = mix_flotio.h lexparse.h mix_encode.h
ASM_O = mixal.o mix_flotio.o lexparse.o mix_encode.o

mix: $(CPU_O) $(COMMON_O)
	cc -o mix $(CPU_O) $(COMMON_O)

mixal: $(ASM_O) $(COMMON_O)
	cc -o mixal $(ASM_O) $(COMMON_O)

%.o: %.c
	cc -c $<
$(CPU_O): $(COMMON_H) $(CPU_H)

$(ASM_O): $(ASM_H) $(COMMON_H)

$(COMMON_O): $(COMMON_H)

clean:
	rm *.o