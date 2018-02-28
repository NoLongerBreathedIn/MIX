# MIX
A MIX emulator. Slightly off-spec.
I/O devices are:

| Number | Type          | Filename (read) | Filename (write) |
|--------|---------------|-----------------|------------------|
| 0-7    | Magnetic tape | `tape[0-7].dev` | `tape[0-7].dev`  |
| 8-15   | Hard drive    | `disk[0-7].dev` | `disk[0-7].dev`  |
| 16     | Card reader   | `card.dev`      |                  |
| 17     | Card punch    |                 | `punch.dev`      |
| 18     | Printer       |                 | `printer.dev`    |
| 19     | Terminal      | (`stdin`)       | (`stdout`)       |
| 20     | Paper tape    | `paper.dev`     | `tapep.dev`      |

I/O is asynchronous, using `<pthread.h>`.

There is a negative address space.
All attempts to access it from positive addresses (including jumps) access instead address 0.
When an I/O action starts from a negative address,
its completion causes an interrupt unless a `JBUS` has occured on that device in the meantime from a negative address.
Use `JRED *+2; JMP X` instead of `JBUS X`, but only if address is negative.
Interrupts occur as described in Knuth.

The floating-point handling is NOT to spec.
We support both one and two word floats. Both are IEEE 754 binary; exponents are 6 or 12 bits long, respectively.
Floating-point tests are `J-`, `JF*`, `JD*` (double-precision, joint `rA` and `rX`),
`JG*` (tests floating point in `rX`), `FCMP`, `GCMP`, and `DCMP`.
FP arithmetic includes `FADD`, `DADD`, `FSUB`, `DSUB`, `FMUL`, `DMUL`, `FDML`, `FDIV`, `DDIV`, and `DFDV`.
`FDML` multiplies `F` by the float in memory and stores it in `D`;
`DFDV` divides `D` by the float in memory and stores it in `F`, leaving `rX` alone.
When do we jump?

| `-`   | `*`  | If positive/greater than | If zero/equal | If negative/less than | If NaN |
|-------|------|--------------------------|---------------|-----------------------|--------|
| `L`   | `N`  |                          |               | √                     |        |
| `E`   | `Z`  |                          | √             |                       |        |
| `G`   | `P`  |                          | √             | √                     |        |
| `GE`  | `PZ` | √                        | √             |                       |        |
| `NE`  | `NZ` | √                        |               | √                     | √      |
| `LE`  | `ZN` |                          | √             | √                     |        |
| `CMP` | `RE` | √                        | √             | √                     |        |
| `NGE` | `IN` |                          |               | √                     | √      |
| `NGL` | `IZ` |                          | √             |                       | √      |
| `NLE` | `IP` | √                        |               |                       | √      |
| `NL`  | `NN` | √                        | √             |                       | √      |
| `GL`  | `PN` | √                        |               | √                     |        |
| `NG`  | `NP` |                          | √             | √                     | √      |
| `NC`  | `NR` |                          |               |                       | √      |

Bytes are 6 bits.

The assembler is exactly to spec. This means that the format is fixed.
It outputs code that can be loaded in to the card reader.
With slight changes in `mixal.c` (in `print_preamble`, move the newline to after the `  CA.` (`JAZ  0,3`),
change the first, second, and fifth lines to ` M [6`, ` Y [6`, and ` D [4` (`IN   14(20)`, `IN   28(20)` and `JBUS 4(20)`),
and change all the 7s in `print_hunk` to 5s) the code becomes loadable onto paper tape, instead.
To modify the emulator to act (at least when booting) as if there is no card reader, it suffices to change the `input(memp, 0, 16); while(!ready(16));` in `main` (`mix_cpu.c`) by replacing the `16` with `20`.

The standard "MIXCII" characters Θ, Φ, and Π are replaced by tilde and open and close brackets.
A final set of eight characters is appended to the end of MIXCII, thus rendering the entire character set
`` ABCDEFGHI~JKLMNOPQR[]STUVWXYZ0123456789.,()+-*/=$<>@;:'`\"&{}|^``.
On input to the computer, lowercase letters are capitalized.
The assembler also works in MIXCII.

One further addition is that `J` is treated as `I7` for purposes of indexing.
This allows return-from-leaf-subroutine to be done with a `JMP 0,7`,
obviating some of the need for self-modifying code.
