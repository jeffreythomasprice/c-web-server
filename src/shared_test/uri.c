#include "../shared/uri.h"

void test(char *input) {
	uri u;
	uri_init(&u);
	uri_parse_cstr(&u, input);
	uri_dealloc(&u);
	log_trace("\n");
}

int main(int argc, char **argv) {
	// these examples all come from the rfc, see https://datatracker.ietf.org/doc/html/rfc3986#section-1.1.2
	test("ftp://ftp.is.co.za/rfc/rfc1808.txt");
	test("http://www.ietf.org/rfc/rfc2396.txt");
	test("ldap://[2001:db8::7]/c=GB?objectClass?one");
	test("mailto:John.Doe@example.com");
	test("news:comp.infosystems.www.servers.unix");
	test("tel:+1-816-555-1212");
	test("telnet://192.0.2.16:80/");
	test("urn:oasis:names:specification:docbook:dtd:xml:4.1.2");

	// examples missing scheme and authority
	test("/");
	test("a");
	test("/a");
	test("/foo/bar");
	test("/foo?ab=cd");
	test("/foo?ab=cd&ef=123");
	test("/foo#/asdf/qwer");
	test("/foo?ab#cd");
	test("?_");
	test("?blah");
	test("#asdf");

	// TODO JEFF negative tests

	return 0;
}