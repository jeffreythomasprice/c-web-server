#include <assert.h>
#include <string.h>

#include "../shared/log.h"
#include "../shared/uri.h"

void assert_cstr_str(char *name, char *expected, string *actual) {
	log_trace("actual \"%s\" = \"%s\", expected = \"%s\"\n", name, actual ? string_get_cstr(actual) : "<null>",
			  expected ? expected : "<null>");
	assert((!expected && !actual) || (expected && actual && !strcmp(expected, string_get_cstr(actual))));
}

void assert_int(char *name, int expected, int actual) {
	log_trace("actual %s = %i, expected = %i\n", name, actual, expected);
	assert(expected == actual);
}

void test(char *input, char *expected_scheme, char *expected_authority, char *expected_userinfo, char *expected_host,
		  char *expected_port_str, int expected_port, char *expected_path, char *expected_query, char *expected_fragment) {
	uri u;
	uri_init(&u);
	log_trace("parsing as uri (expected to succeed): \"%s\"\n", input);
	assert(!uri_parse_cstr(&u, input));
	assert_cstr_str("scheme", expected_scheme, uri_get_scheme(&u));
	assert_cstr_str("authority", expected_authority, uri_get_authority(&u));
	assert_cstr_str("userinfo", expected_userinfo, uri_get_userinfo(&u));
	assert_cstr_str("host", expected_host, uri_get_host(&u));
	assert_cstr_str("port", expected_port_str, uri_get_port_str(&u));
	assert_int("port", expected_port, uri_get_port(&u));
	assert_cstr_str("path", expected_path, uri_get_path(&u));
	assert_cstr_str("query", expected_query, uri_get_query(&u));
	assert_cstr_str("fragment", expected_fragment, uri_get_fragment(&u));
	uri_dealloc(&u);
	log_trace("\n");
}

void test_fail(char *input) {
	uri u;
	uri_init(&u);
	log_trace("parsing as uri (expected to fail): \"%s\"\n", input);
	assert(uri_parse_cstr(&u, input));
	uri_dealloc(&u);
	log_trace("\n");
}

int main(int argc, char **argv) {
	// these examples all come from the rfc, see https://datatracker.ietf.org/doc/html/rfc3986#section-1.1.2
	test("ftp://ftp.is.co.za/rfc/rfc1808.txt", "ftp", "ftp.is.co.za", NULL, "ftp.is.co.za", NULL, 0, "/rfc/rfc1808.txt", NULL, NULL);
	test("http://www.ietf.org/rfc/rfc2396.txt", "http", "www.ietf.org", NULL, "www.ietf.org", NULL, 0, "/rfc/rfc2396.txt", NULL, NULL);
	test("ldap://[2001:db8::7]/c=GB?objectClass?one", "ldap", "[2001:db8::7]", NULL, "[2001:db8::7]", NULL, 0, "/c=GB", "objectClass?one",
		 NULL);
	test("mailto:John.Doe@example.com", "mailto", "John.Doe@example.com", "John.Doe", "example.com", NULL, 0, NULL, NULL, NULL);
	test("news:comp.infosystems.www.servers.unix", "news", "comp.infosystems.www.servers.unix", NULL, "comp.infosystems.www.servers.unix",
		 NULL, 0, NULL, NULL, NULL);
	test("tel:+1-816-555-1212", "tel", "+1-816-555-1212", NULL, "+1-816-555-1212", NULL, 0, NULL, NULL, NULL);
	test("telnet://192.0.2.16:80/", "telnet", "192.0.2.16:80", NULL, "192.0.2.16", "80", 80, "/", NULL, NULL);
	test("urn:oasis:names:specification:docbook:dtd:xml:4.1.2", "urn", "oasis:names:specification:docbook:dtd:xml:4.1.2", NULL,
		 "oasis:names:specification:docbook:dtd:xml:4.1.2", NULL, 0, NULL, NULL, NULL);

	// examples missing scheme and authority
	test("/", NULL, NULL, NULL, NULL, NULL, 0, "/", NULL, NULL);
	test("a", NULL, "a", NULL, "a", NULL, 0, NULL, NULL, NULL);
	test("/a", NULL, NULL, NULL, NULL, NULL, 0, "/a", NULL, NULL);
	test("/foo/bar", NULL, NULL, NULL, NULL, NULL, 0, "/foo/bar", NULL, NULL);
	test("/foo?ab=cd", NULL, NULL, NULL, NULL, NULL, 0, "/foo", "ab=cd", NULL);
	test("/foo?ab=cd&ef=123", NULL, NULL, NULL, NULL, NULL, 0, "/foo", "ab=cd&ef=123", NULL);
	test("/foo#/asdf/qwer", NULL, NULL, NULL, NULL, NULL, 0, "/foo", NULL, "/asdf/qwer");
	test("/foo?ab#cd", NULL, NULL, NULL, NULL, NULL, 0, "/foo", "ab", "cd");
	test("?_", NULL, NULL, NULL, NULL, NULL, 0, NULL, "_", NULL);
	test("?blah", NULL, NULL, NULL, NULL, NULL, 0, NULL, "blah", NULL);
	test("#asdf", NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, "asdf");
	test("#a", NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, "a");

	// negative tests
	test_fail("");
	// TODO JEFF all whitespace test segfaults?
	test_fail("    ");
	test_fail("?");
	test_fail("#");

	return 0;
}