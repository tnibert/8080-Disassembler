#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
    *codebuffer is a valid pointer to 8080 assembly code
    pc is the current offset into the code

    returns the number of bytes of the op
*/

typedef struct ConditionCodes {
    /*
    z - zero, set to 1 when result is equal to zero
    s - sign, set to 1 when bit 7 (most significant bit) of math instruction is set
    p - parity, set when the answer has even parity, clear when odd parity
    cy - carry, set to 1 when instruction resulted in a carry out or borrow into the higher order bit
    ac - auxillary carry, used for Binary Coded Decimal math (space invaders doesn't use)
    ^ used by conditional branching, ex) jz branches if z flag is set
    */
    uint8_t    z:1;
    uint8_t    s:1;
    uint8_t    p:1;
    uint8_t    cy:1;
    uint8_t    ac:1;
    uint8_t    pad:3;
} ConditionCodes;

typedef struct State8080 {
    uint8_t    a;       // register A - the accumulator
    uint8_t    b;
    uint8_t    c;
    uint8_t    d;
    uint8_t    e;
    uint8_t    h;
    uint8_t    l;
    uint16_t   sp;
    uint16_t   pc;
    uint8_t    *memory;
    struct     ConditionCodes cc;
    uint8_t    int_enable;
} State8080;

int Parity(int x, int size)
{
    // hmmmmmmmmmmmm
    int i;
    int p = 0;
    x = (x & ((1<<size)-1));
    for (i=0; i<size; i++)
    {
        if (x & 0x1) p++;
        x = x >> 1;
    }
    return (0 == (p & 0x1));
}

void UnimplementedInstruction(State8080* state)
{
    state->pc--;    // decrement as we have advanced one
    printf ("Error: Unimplemented instruction\n");
    //Disassemble8080Op(state->memory, state->pc);
    //printf("\n");
    exit(1);
}

int Emulate8080Op(State8080* state)
{
    unsigned char *opcode = &state->memory[state->pc];

    switch(*opcode)
    {
        /*case 0x00: UnimplementedInstruction(state); break;
        case 0x01: UnimplementedInstruction(state); break;
        case 0x02: UnimplementedInstruction(state); break;
        case 0x03: UnimplementedInstruction(state); break;
        case 0x04: UnimplementedInstruction(state); break;
        /*....
        case 0xfe: UnimplementedInstruction(state); break;
        case 0xff: UnimplementedInstruction(state); break;
        */
        case 0x00: break;                   //NOP is easy!
        case 0x01:                          //LXI   B,word
            state->c = opcode[1];
            state->b = opcode[2];
            state->pc += 2;                  //Advance 2 more bytes
            break;
        /*....*/
        case 0x41: state->b = state->c; break;    //MOV B,C
        case 0x42: state->b = state->d; break;    //MOV B,D
        case 0x43: state->b = state->e; break;    //MOV B,E

        // next two are register form arithmetic
        case 0x80:                                //ADD B
        {
            //uses flags
            // do the math with higher precision so we can capture the
            // carry out
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;

            // Zero flag: if the result is zero,
            // set the flag to zero
            // else clear the flag
            if ((answer & 0xff) == 0)
                state->cc.z = 1;
            else
                state->cc.z = 0;

            // Sign flag: if bit 7 is set,
            // set the sign flag
            // else clear the sign flag
            if (answer & 0x80)
                state->cc.s = 1;
            else
                state->cc.s = 0;

            // Carry flag
            if (answer > 0xff)
                state->cc.cy = 1;
            else
                state->cc.cy = 0;

            // Parity is handled by a subroutine
            //state->cc.p = Parity( answer & 0xff);

            state->a = answer & 0xff;
        } break;

        //The code for ADD can be condensed into the ADD function
        case 0x81:                                      //ADD C
            Add(state, state->c);
            break;

        // memory form... confusing
        case 0x86:                                      //ADD M
            // wtf is this line calculating?
            ;   // empty statement to avoid compilation error
            uint16_t offset = (state->h<<8) | (state->l);
            // does this indicate that we need to pass uint16_t to Add?
            // no... it can't be because state->memory[offset] only gives one byte
            //uint16_t answer = (uint16_t) state->a + state->memory[offset];
            Add(state, state->memory[offset]);
            break;

        /* next is immediate form
           source of the addend is the byte after the instruction */
        case 0xC6:      //ADI byte
            Add(state, opcode[1]);
            state->pc += 1;
            break;

        default: UnimplementedInstruction(state); break;
    }
    state->pc+=1;  //for the opcode
}

void Add(State8080 *state, uint8_t addend)
{
    /*
        perform addition and set flags

        NOTES: for addition arithmetic instructions:
        In the carry variants (ADC, ACI, SBB, SUI) you use the carry bit in the calculation as indicated in the data book.

        INX and DCX effect register pairs, these instructions do not effect the flags.

        DAD is another register pair instruction, it only effects the carry flag

        INR and DCR do not effect the carry flag
    */
    uint16_t answer = (uint16_t) state->a + (uint16_t) addend;
    state->cc.z = ((answer & 0xff) == 0);
    state->cc.s = ((answer & 0x80) != 0);
    state->cc.cy = (answer > 0xff);
    //state->cc.p = Parity(answer&0xff);
    // final result stored in register A
    state->a = answer & 0xff;
}

int Disassemble8080Op(unsigned char *codebuffer, int pc)
{
    unsigned char *code = &codebuffer[pc];
    int opbytes = 1;
    printf ("%04x ", pc);
    switch (*code)
    {
        // http://www.emulator101.com/8080-by-opcode.html
        case 0x00: printf("NOP"); break;
        case 0x01: printf("LXI    B,#$%02x%02x", code[2], code[1]); opbytes=3; break;
        case 0x02: printf("STAX   B"); break;
        case 0x03: printf("INX    B"); break;
        case 0x04: printf("INR    B"); break;
        case 0x05: printf("DCR    B"); break;
        case 0x06: printf("MVI    B,#$%02x", code[1]); opbytes=2; break;
        case 0x07: printf("RLC"); break;
        case 0x08: printf("NOP"); break;
        case 0x09: printf("DAD    B"); break;
        case 0x0a: printf("LDAX   B"); break;
        case 0x0b: printf("DCX    B"); break;
        case 0x0c: printf("INR    C"); break;
        case 0x0d: printf("DCR    C"); break;
        case 0x0e: printf("MVI    C,#$%02x", code[1]); opbytes=2; break;
        case 0x0f: printf("RRC"); break;
        // case 0x10 N/A
        case 0x11: printf("LXI    D,#$%02x%02x", code[2], code[1]); opbytes=3; break;
        case 0x12: printf("STAX   D"); break;
        case 0x13: printf("INX    D"); break;
        case 0x14: printf("INR    D"); break;
        case 0x15: printf("DCR    D"); break;
        case 0x16: printf("MVI    D,#$%02x", code[1]); opbytes = 2; break;
        case 0x17: printf("RAL"); break;
        // 0x18 N/A
        case 0x19: printf("DAD    D"); break;
        case 0x1a: printf("LDAX   D"); break;
        case 0x1b: printf("DCX    D"); break;
        case 0x1c: printf("INR    E"); break;
        case 0x1d: printf("DCR    E"); break;
        case 0x1e: printf("MVI    E,#$%02x", code[1]); opbytes=2; break;
        case 0x1f: printf("RAR"); break;
        case 0x20: printf("RIM"); break;
        case 0x21: printf("LXI    H,#$%02x%02x", code[2], code[1]); opbytes=3; break;
        case 0x22: printf("SHLD   $%02x%02x", code[2], code[1]); opbytes=3; break;
        /* ........
        #0x..*/
        case 0x3e: printf("MVI    A,#0x%02x", code[1]); opbytes = 2; break;
        /* ........
        $adr
         */
        case 0xc3: printf("JMP    $%02x%02x",code[2],code[1]); opbytes = 3; break;
        /* ........ */
        default: printf("-"); break;
    }

    printf("\n");

    return opbytes;
}

int main (int argc, char**argv)
{
    FILE *f= fopen(argv[1], "rb");
    if (f==NULL)
    {
        printf("error: Couldn't open %s\n", argv[1]);
        exit(1);
    }

    //Get the file size and read it into a memory buffer
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    unsigned char *buffer=malloc(fsize);

    fread(buffer, fsize, 1, f);
    fclose(f);

    // create and init state machine
    State8080 * emustate = calloc(1, sizeof(State8080));
    emustate->memory = malloc(0x10000);  //16K

    emustate->pc = 0;

    //while (pc < fsize)
    //{
    //    pc += Disassemble8080Op(buffer, pc);
    //}
    for(int done=0; done == 0;)
    {
        done = Emulate8080Op(emustate);
    }
    return 0;
}
