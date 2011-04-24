#ifndef BIN2HEX
#define BIN2HEX

class bin2hex
{
private:
	char * buf;
public:
	bin2hex();
	~bin2hex();
	const char * operator()(unsigned char input);
};
#endif
