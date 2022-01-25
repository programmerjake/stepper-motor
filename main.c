#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <math.h>
#include <stdlib.h>

static void SetFastClock() // set clock to 8 MHz
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        CLKPR = _BV(CLKPCE);
        CLKPR = 0;
    }
}

static uint8_t reverse(uint8_t v)
{
    uint8_t retval;
	__asm__ volatile (
        "bst %1, 0" "\n\t"
        "bld %0, 7" "\n\t"
        "bst %1, 1" "\n\t"
        "bld %0, 6" "\n\t"
        "bst %1, 2" "\n\t"
        "bld %0, 5" "\n\t"
        "bst %1, 3" "\n\t"
        "bld %0, 4" "\n\t"
        "bst %1, 4" "\n\t"
        "bld %0, 3" "\n\t"
        "bst %1, 5" "\n\t"
        "bld %0, 2" "\n\t"
        "bst %1, 6" "\n\t"
        "bld %0, 1" "\n\t"
        "bst %1, 7" "\n\t"
        "bld %0, 0"
		: "=&r" (retval)
		: "r" (v)
        : "cc"
	);
	return retval;
}

static uint8_t motorditherval = 0;

static void SetMotorPosition(int16_t motorposition)
{
    motorposition &= 0x7FF;
    uint8_t n = 0, e = 0, s = 0, w = 0;
    if((motorposition >> 8) < 1)
    {
        n = 0xFF;
        e = motorposition & 0xFF;
    }
    else if((motorposition >> 8) < 2)
    {
        n = ~motorposition & 0xFF;
        e = 0xFF;
    }
    else if((motorposition >> 8) < 3)
    {
        e = 0xFF;
        s = motorposition & 0xFF;
    }
    else if((motorposition >> 8) < 4)
    {
        e = ~motorposition & 0xFF;
        s = 0xFF;
    }
    else if((motorposition >> 8) < 5)
    {
        s = 0xFF;
        w = motorposition & 0xFF;
    }
    else if((motorposition >> 8) < 6)
    {
        s = ~motorposition & 0xFF;
        w = 0xFF;
    }
    else if((motorposition >> 8) < 7)
    {
        w = 0xFF;
        n = motorposition & 0xFF;
    }
    else
    {
        w = ~motorposition & 0xFF;
        n = 0xFF;
    }
    uint8_t reversedmotorditherval = reverse(motorditherval);
    uint8_t pv = PORTB;
    pv &= ~_BV(PORTB0) & ~_BV(PORTB1) & ~_BV(PORTB2) & ~_BV(PORTB3);
    if(n > reversedmotorditherval)
        pv |= _BV(PORTB0);
    if(e > reversedmotorditherval)
        pv |= _BV(PORTB1);
    if(s > reversedmotorditherval)
        pv |= _BV(PORTB2);
    if(w > reversedmotorditherval)
        pv |= _BV(PORTB3);
    PORTB = pv;
    motorditherval++;
}

#define MaxMotorSpeed 0x44000L

int main(void)
{
    DDRB = _BV(DDB0) | _BV(DDB1) | _BV(DDB2) | _BV(DDB3) | _BV(DDB4) | _BV(DDB5); // all outputs
    PORTB = _BV(PORTB5);

    SetFastClock();

    uint32_t motorposition = 0;
    int32_t motorspeed = 0;
    int16_t deltamotorspeed = 1;

    while(1)
    {
        motorspeed += deltamotorspeed;
        if(motorspeed >= MaxMotorSpeed)
            deltamotorspeed = -abs(deltamotorspeed);
        else if(motorspeed <= -MaxMotorSpeed)
            deltamotorspeed = abs(deltamotorspeed);

        if(motorspeed > 0)
            motorposition += (motorspeed >> 4) * (motorspeed >> 4) >> 7;
        else
            motorposition -= (motorspeed >> 4) * (motorspeed >> 4) >> 7;
        SetMotorPosition(motorposition >> 16);
    }

    return 0;
}
