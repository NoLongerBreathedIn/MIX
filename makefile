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

FAKE_H = mixal.h mix_cpu.h

mix: $(CPU_O) $(COMMON_O)
	cc -pthread -o mix $(CPU_O) $(COMMON_O)

mixal: $(ASM_O) $(COMMON_O)
	cc -o mixal $(ASM_O) $(COMMON_O)

%.o: %.c
	cc -c $<

%.o: %.h

mix_cpu.o: $(COMMON_H) mix_io.h mix_decode.h
mix_io.o: atomic_queue.h
mixal.o: mix_types.h $(ASM_H)

mix_float.h mix_io.h lexparse.h mix_encode.h: mix_types.h
	touch $@

mix_flotio.h: mix_float.h
	touch $@

$(FAKE_H):
	touch $@

.INTERMEDIATE: $(FAKE_H)

clean:
	rm *.o
