# MIX
A MIX emulator. Slightly off-spec.
I/O devices are:

| Number | Type          | Filename (read) | Filename (write) |
|--------|---------------|-----------------|------------------|
| 0-7    | Magnetic tape | `tape[0-8].dev` | `tapei.dev`      |
| 8-15   | Hard drive    | `disk[0-8].dev` | `diski.dev`      |
| 16     | Card reader   | `card.dev`      |                  |
| 17     | Card punch    |                 | `punch.dev`      |
| 18     | Printer       |                 | `printer.dev`    |
| 19     | Terminal      | (`stdin`)       | (`stdout`)       |
| 20     | Paper tape    | `paper.dev`     | `tapep.dev`      |

I/O is asynchronous, using `<pthread.h>`.

There is a negative address space.
All attempts to access it from positive addresses (including jumps) access instead address 0.
When an I/O action starts from a negative address,
its completion causes an interrupt unless a `JBUS` has occured on that device from a negative address.
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
