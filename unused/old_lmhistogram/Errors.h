#ifndef HdrErrors_h
#define HdrErrors_h
#define		OK	0
#define		E_FILE_NOT_READ	-1
#define		E_TAG_NOT_FOUND -2
#define		E_FILE_ALREADY_OPEN -3
#define		E_COULD_NOT_OPEN_FILE -4
#define		E_FILE_NOT_WRITE	-5
#define		E_NOT_AN_INIT		-6
#define		E_NOT_A_LONG		-7
#define		E_NOT_A_FLOAT		-8
#define		E_NOT_A_DOUBLE		-9

static char *HdrErrors[] =
{
	"OK",
	"Attempt to read a file that is not open as read",
	"Specified tag was not found",
	"File already open",
	"Could not open specified file",
	"Attempt to write to a file that is not open as write",
	"Attempt to read a none decimal digit in readint function",
	"Attempt to read a none decimal digit in readlong function",
	"Attempt to read a none decimal digit in readfloat function",
	"Attempt to read a none decimal digit in readdouble function"
};
#endif