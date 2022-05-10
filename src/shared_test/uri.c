#include "../shared/uri.h"

void test(char *input) {
	uri u;
	uri_init(&u);
	uri_parse_cstr(&u, input);
	uri_dealloc(&u);
}

int main(int argc, char **argv) {
	// these examples all come from the rfc, see https://datatracker.ietf.org/doc/html/rfc3986#section-1.1.2
	test("ftp://ftp.is.co.za/rfc/rfc1808.txt");
	/*
	TODO JEFF rest of the rfc examples
	http://www.ietf.org/rfc/rfc2396.txt
	ldap://[2001:db8::7]/c=GB?objectClass?one
	mailto:John.Doe@example.com
	news:comp.infosystems.www.servers.unix
	tel:+1-816-555-1212
	telnet://192.0.2.16:80/
	urn:oasis:names:specification:docbook:dtd:xml:4.1.2
	*/
	return 0;
}