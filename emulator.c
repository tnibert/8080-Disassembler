#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
    *codebuffer is a valid pointer to 8080 assembly code
    pc is the current offset into the code

    returns the number of bytes of the op
*/

typedef struct ConditionCodes {
    uint8_t    z:1;
    uint8_t    s:1;
    uint8_t    p:1;
    uint8_t    cy:1;
    uint8_t    ac:1;
    uint8_t    pad:3;
} ConditionCodes;

typedef struct State8080 {
    uint8_t    a;
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
        default: UnimplementedInstruction(state); break;
    }
    state->pc+=1;  //for the opcode
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
