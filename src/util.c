#include "../include/util.h"
#include "../include/extern.h"
#include "../include/proc.h"

extern TaskData pInitTD;
extern Proc **pproc_tab;


unsigned char inportb (unsigned short _port)
{
	unsigned char rv;

	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
	return rv;
}

void outportb (unsigned short _port, unsigned char _data)
{
	__asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

void halt()
{
	DisableInterrupts();
	__asm("hlt");
}

void EnableInterrupts()
{
	__asm("sti");
	intsEnabled = true;
}

void DisableInterrupts()
{
	__asm("cli");
	intsEnabled = false;
}

void reboot(void)
{
	unsigned temp;

	DisableInterrupts();

	/* flush the keyboard controller */
	do
    {
		temp = inportb(0x64);
		if((temp & 0x01) != 0)
		{
			(void)inportb(0x60);
			continue;
		}
	} while((temp & 0x02) != 0);

	/* pulse the CPU reset line */
	outportb(0x64, 0xFE);
	// ...and if that didn't work, try again.

	//If it doesn't work...
	outportb(0x64, 0xFE);

	//try something else...

	//just freeze
	__asm("hlt");
}

void do_panic(const char *str, const char * func, const char * file, int line)
{
	DisableInterrupts();

	debug_printf("\n\nPanic! %s\n%s(), File %s, Line %d", str, func, file, line);
	__asm("hlt");
}

void memcpy(void *dest, void *src, int num)
{
	char *d = (char*)dest;
	char *s = (char*)src;

	int i;
	for(i = 0; i < num; i++, d++, s++)
		*d = *s;
}

void memset(void *ptr, int c, int num)
{
	UINT i;
	for(i = 0; i < num ; i++ )
		((char*) ptr)[i] = (char)c;
}

void clrscr()
{
	char * buf = (char*)pInitTD.TD_VirtVid;
	//clear lines (leaving every other byte for color info)
	UINT i;
	for(i = 0; i < pInitTD.TD_nLines*(pInitTD.TD_nColumns*2); i+=2) {
		buf[i] = 0;
		buf[i+1] = 0x7; //restore color byte
	}
}

void putchar(char c)
{
	UINT i;

	//get a pointer to the taskdata struct
	//given pCurrentTSS

	TaskData *td = 0;//pCurrentTSS->owner;

	//now use the task data struct to get write the text to the virtual buffer using
	//the position and color/status/format/mode settings in the task data struct

	//put char at offset  located at td->Curx and Cury
	//increment x, check if x is greated than nColumns
	//if so, x = 0, increment y.  check if y is greater than nRows..if so scroll down one line

	//assume the x and y coordinates are correct
	UINT offset = (td->TD_CurY * td->TD_nColumns * 2) + (td->TD_CurX*2);

	char *vid = (char*)(td->TD_VirtVid);

	if(c == '\n')
		goto newline;
	else if(c == '\t')
	{
		for(i = 0; i < 5; i++)
			putchar(' ');

		goto updatepos;
	}
	else if(c == '\0')
		return;


	vid[offset] = c;

updatepos:

	td->TD_CurX++;

	if(td->TD_CurX > td->TD_nColumns)
	{
newline:
		td->TD_CurX = 0;
		if(td->TD_CurY <= td->TD_nLines)
			td->TD_CurY++;
		else
		{
			//scroll down 1 lines
			offset = (1 * td->TD_nColumns * 2);
			memcpy(vid, vid+offset, ((td->TD_nColumns + td->TD_nLines) * 2)-offset);

			td->TD_CurY = td->TD_nLines;

		}
	}
}

void printf(const char *format, ...)
{
	if(format == NULL)
		return;

	char **arg = (char **) &format;
    int c;
    char buf[20];

    arg++;
    while ((c = *format++) != 0)
    {
		if(c != '%')
	    	putchar(c);
		else
		{
			char *p;

			c = *format++;
			switch (c)
			{
				case 'd':
				case 'u':
			 	case 'x':
				{
					itoa(buf, c, *((int *) arg++));
					p = buf;
					goto string;
					break;
				}
			 	case 's':
				{
					p = *arg++;
					if(!p)
						p = "(null)";
				}
			 	string:
					while (*p)
						putchar(*p++);
					break;
				default:
					putchar(*((int *) arg++));
					break;
			}
		}
    }

	return;
}

void itoa (char *buf, int base, int d)
{
    char *p = buf;
    char *p1, *p2;
    unsigned long ud = d;
    int divisor = 10;

    /* If %d is specified and D is minus, put `-' in the head.  */
    if (base == 'd' && d < 0)
    {
		*p++ = '-';
		buf++;
		ud = -d;
    }
    else if (base == 'x')
		divisor = 16;

    /* Divide US by divisor until UD == 0.  */
    do
    {
		int remainder = ud % divisor;

		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= divisor);


	/* hex values */
	if(base == 'x')
	{
		*p++ = 'x';
		*p++ = '0';
	}

	/* Reverse it */
    *p = 0;
    p1 = buf;
    p2 = p - 1;
    while (p1 < p2)
    {
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
    }
}

void ultoa (char *buf, int base, ULONG d)
{
    char *p = buf;
    char *p1, *p2;
    unsigned long ud = d;
    int divisor = 10;

    /* If %d is specified and D is minus, put `-' in the head.  */
    if (base == 'd' && d < 0)
    {
		*p++ = '-';
		buf++;
		ud = -d;
    }
    else if (base == 'x')
		divisor = 16;

    /* Divide UD by DIVISOR until UD == 0.  */
    do
    {
		int remainder = ud % divisor;

		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= divisor);

    /* Terminate BUF.  */
    *p = 0;

    /* Reverse BUF.  */
    p1 = buf;
    p2 = p - 1;
    while (p1 < p2)
    {
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
    }
}

UINT strlen(const char* str)
{
	UINT i = 0;

	while(1)
	{
		if(str[i] == '\0')
			break;
		i++;
	}

	return i;
}

char * strcpy(char *dest, const char *src)
{
	UINT i = 0;

	do
	{
		dest[i] = src[i];
		i++;

	} while(src[i] != '\0');


	return dest;
}

char * strcat(const char * str1, const char * str2)
{
	if(str1 == NULL) {
		return str2;
	} else if(str2 == NULL) {
		return str1;
	} else if(str1 == str2 == NULL) {
		return NULL;
	}

	int len1 = strlen(str1);
	int len2 = strlen(str2);
	int len = len1 + len2 + 1;
	char * buffer = (char*)kmalloc(len);
	char * ptr = buffer;
	int i = 0;
	while(i < len) {
		if(i > len1)
			ptr[i] = str1[i];
		else
			ptr[i] = str2[i];
		i++;
	}
	ptr[len] = '\0';
	return buffer;
}

void debug_putchar(char c)
{
	UINT i;

	TaskData * td;

	if(pInitTD.TD_ExitCode == 0x9D)
		td = &pInitTD;
	else
		td = pproc_tab[0]->td;

	UINT offset = (td->TD_CurY * td->TD_nColumns * 2) + (td->TD_CurX*2);

	char *vid = (char*)(td->TD_VirtVid);

	if(c == '\n')
		goto newline;
	else if(c == '\t')
	{
		for(i = 0; i < 5; i++)
			debug_putchar(' ');

		goto updatepos;
	}
	else if(c == '\0')
	{
		return;
	}


	vid[offset] = c;

updatepos:

	td->TD_CurX++;

	if(td->TD_CurX > td->TD_nColumns)
	{
newline:
		td->TD_CurX = 0;
		td->TD_CurY++;
		if(td->TD_CurY < td->TD_nLines)
			{}//td->TD_CurY++;
		else
		{
			//scroll down 1 lines
			//offset = (1 * td->TD_nColumns * 2);
			//memcpy(vid, vid+offset, ((td->TD_nColumns + td->TD_nLines) * 2)-offset);

			//td->TD_CurY = td->TD_nLines;

			Scroll(td);
			td->TD_CurY = td->TD_nLines-1;

		}
	}
}

void dprintf(const char *format, ...)
{
	debug_printf(format);
}

void debug_printf(const char *format, ...)
{
	if(format == NULL)
		return;

	char ints_enable = intsEnabled;
	DisableInterrupts();


	char **arg = (char **) &format;
	int c;
	char buf[20];

	arg++;
	while ((c = *format++) != 0)
	{
		if(c != '%')
			debug_putchar(c);
		else
		{
			char *p;

			c = *format++;
			switch (c)
			{
				case 'd':
				{
					ultoa(buf, c, *((ULONG *) arg++));
					p = buf;
					goto string;
					break;
				}
				case 'u':
				case 'x':
				{
					itoa(buf, c, *((int *) arg++));
					p = buf;
					goto string;
					break;
				}
				case 's':
					p = *arg++;
					if (! p)
						p = "(null)";

				string:
					while (*p)
						debug_putchar(*p++);
					break;

				default:
					debug_putchar(*((int *) arg++));
					break;
			}
		}
	}

	if(ints_enable)
		EnableInterrupts();

	return;
}

#define FILL_DWORD (0x00200020) | (ATTRIB(BLACK, GRAY) <<24) | ((ATTRIB(BLACK, GRAY)<<8))
static void Scroll(TaskData * td)
{
	unsigned int* v;
	int i, n = (((td->TD_nLines-1) * td->TD_nColumns * 2) / 4);

	int NUM_DWORDS_PER_LINE = ((td->TD_nColumns*2)/4);

	UINT fill = FILL_DWORD;

	// Move lines 1..NUMROWS-1 up one position.
	for ( v = (unsigned int*)td->TD_VirtVid, i = 0; i < n; ++i )
	{
		*v = *(v + NUM_DWORDS_PER_LINE);
		++v;
	}

	for ( v = (unsigned int*)td->TD_VirtVid + n, i = 0; i < NUM_DWORDS_PER_LINE; ++i )
		*v++ = fill;
}


//This should be all long doubles
unsigned long pow(ULONG x, ULONG y)
{
	if(y == 0)
		return 1;

	ULONG res = x;
	y--;

	while(y)
	{
		res = res*x;
		y--;
	}

	return res;
}

void wait(int x)
{
	while(x--);
}

/*==========================================================================
Function: SetBit
INPUT:
		t - address of array to be used
		loc - number of bit to be set(0 based and big-endian)
*=========================================================================*/
void inline SetBit(void *t, UINT loc)
{
	if(!t)
		return;

	int byte = (loc / 8);
	int mod = (loc % 8);

	/*if(mod == 0 && loc != 0)
		mod = 8;
	else if(mod == 0)// && loc == 0)
		mod = 0;*/

	unsigned long x = pow(2, mod);

	char *a = (char *)(t + (byte));

	a[0] |= x;
}

/*==========================================================================
Function: GetBitState
INPUT:
		t - address of array to be used
		loc - number of bit to be set(0 based and big-endian)
OUTPUT:
		returns the status of the bit(0 not set, 1 set)
*=========================================================================*/
char inline GetBitState(void *t, UINT loc)
{
	if(!t)
		return 0;

	char set = 0;

	int byte = (loc / 8);
	int mod = (loc % 8);

	char* data = (char*)(t + (byte));

	/*if(mod == 0 && loc != 0)
		mod = 8;
	else if(mod == 0)// && loc == 0)
		mod = 0;*/

	unsigned char x = (unsigned char)pow(2, mod);

	if(data[0] & x)
		set = 1;

	return set;
}

/*==========================================================================
Function: ClearBit
INPUT:
		t - address of array to be used
		loc - number of bit to be set(0 based and big-endian)
*=========================================================================*/
void inline ClearBit(void *t, UINT loc)
{
	if(!t)
		return;

	//loc = loc - 1;

	int byte = (loc / 8);
	int mod = (loc % 8);

	/*if(mod == 0 && loc != 0)
		mod = 8;
	else if(mod == 0)// && loc == 0)
		mod = 0;*/

	unsigned char x = (unsigned char)pow(2, mod);

	char *a = (char *)(t + (byte));

	x = (255-x);

	a[0] &= x;
}
