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

void test_decode() {
	string s, s2;
	string_init(&s);
	string_init(&s2);
	string_set_cstr(&s, "\%30\%31\%32\%33");
	uri_append_decoded_str(&s2, &s);
	assert(!string_compare_cstr(&s2, "0123", STRING_COMPARE_CASE_SENSITIVE));
	uri_append_decoded_str(&s, &s);
	assert(!string_compare_cstr(&s, "\%30\%31\%32\%330123", STRING_COMPARE_CASE_SENSITIVE));
	uri_append_decoded_cstr(&s, "\%3cfoo\%3e");
	assert(!string_compare_cstr(&s, "\%30\%31\%32\%330123<foo>", STRING_COMPARE_CASE_SENSITIVE));
	string_clear(&s);
	uri_append_decoded_cstr(&s, "\%Fe\%fE\%fe\%FE");
	char tmp[5] = {0xfe, 0xfe, 0xfe, 0xfe, 0};
	assert(!string_compare_cstr(&s, tmp, STRING_COMPARE_CASE_SENSITIVE));
	string_dealloc(&s);
	string_dealloc(&s2);
}

void test_encode() {
	string s, s2;
	string_init(&s);
	string_init(&s2);
	string_set_cstr(&s, "foo bar baz");
	uri_append_encoded_str(&s2, &s);
	assert(!string_compare_cstr(&s2, "foo\%20bar\%20baz", STRING_COMPARE_CASE_SENSITIVE));
	uri_append_encoded_str(&s, &s);
	assert(!string_compare_cstr(&s, "foo bar bazfoo\%20bar\%20baz", STRING_COMPARE_CASE_SENSITIVE));
	uri_append_encoded_cstr(&s, "\t\tasdf  ");
	assert(!string_compare_cstr(&s, "foo bar bazfoo\%20bar\%20baz\%09\%09asdf\%20\%20", STRING_COMPARE_CASE_SENSITIVE));
	// TODO JEFF needs more tests for encode, @ in userinfo, : in host, # or ? in path, # in query
	string_dealloc(&s);
	string_dealloc(&s2);
}

void test_parse_pass(char *input, char *expected_scheme, char *expected_authority, char *expected_userinfo, char *expected_host,
					 char *expected_port_str, int expected_port, char *expected_path, char *expected_query, char *expected_fragment,
					 char *expected_recreation) {
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

	string s;
	string_init(&s);
	uri_append_to_string(&u, &s);
	assert_cstr_str("re-created original string", expected_recreation, &s);
	string_dealloc(&s);

	uri_dealloc(&u);

	log_trace("\n");
}

void test_parse_fail(char *input) {
	uri u;
	uri_init(&u);
	log_trace("parsing as uri (expected to fail): \"%s\"\n", input);
	assert(uri_parse_cstr(&u, input));
	uri_dealloc(&u);
	log_trace("\n");
}

int main(int argc, char **argv) {
	test_decode();
	test_encode();

	// these examples all come from the rfc, see https://datatracker.ietf.org/doc/html/rfc3986#section-1.1.2
	test_parse_pass("ftp://ftp.is.co.za/rfc/rfc1808.txt", "ftp://", "ftp.is.co.za", NULL, "ftp.is.co.za", NULL, 0, "/rfc/rfc1808.txt", NULL,
					NULL, "ftp://ftp.is.co.za/rfc/rfc1808.txt");
	test_parse_pass("http://www.ietf.org/rfc/rfc2396.txt", "http://", "www.ietf.org", NULL, "www.ietf.org", NULL, 0, "/rfc/rfc2396.txt",
					NULL, NULL, "http://www.ietf.org/rfc/rfc2396.txt");
	test_parse_pass("ldap://[2001:db8::7]/c=GB?objectClass?one", "ldap://", "[2001:db8::7]", NULL, "[2001:db8::7]", NULL, 0, "/c=GB",
					"objectClass?one", NULL, "ldap://[2001:db8::7]/c=GB?objectClass?one");
	test_parse_pass("mailto:John.Doe@example.com", "mailto:", "John.Doe@example.com", "John.Doe", "example.com", NULL, 0, NULL, NULL, NULL,
					"mailto:John.Doe@example.com");
	test_parse_pass("news:comp.infosystems.www.servers.unix", "news:", "comp.infosystems.www.servers.unix", NULL,
					"comp.infosystems.www.servers.unix", NULL, 0, NULL, NULL, NULL, "news:comp.infosystems.www.servers.unix");
	test_parse_pass("tel:+1-816-555-1212", "tel:", "+1-816-555-1212", NULL, "+1-816-555-1212", NULL, 0, NULL, NULL, NULL,
					"tel:+1-816-555-1212");
	test_parse_pass("telnet://192.0.2.16:80/", "telnet://", "192.0.2.16:80", NULL, "192.0.2.16", "80", 80, "/", NULL, NULL,
					"telnet://192.0.2.16:80/");
	test_parse_pass("urn:oasis:names:specification:docbook:dtd:xml:4.1.2", "urn:", "oasis:names:specification:docbook:dtd:xml:4.1.2", NULL,
					"oasis:names:specification:docbook:dtd:xml:4.1.2", NULL, 0, NULL, NULL, NULL,
					"urn:oasis:names:specification:docbook:dtd:xml:4.1.2");

	// examples missing scheme and authority
	test_parse_pass("/", NULL, NULL, NULL, NULL, NULL, 0, "/", NULL, NULL, "/");
	test_parse_pass("a", NULL, "a", NULL, "a", NULL, 0, NULL, NULL, NULL, "a");
	test_parse_pass("/a", NULL, NULL, NULL, NULL, NULL, 0, "/a", NULL, NULL, "/a");
	test_parse_pass("/foo/bar", NULL, NULL, NULL, NULL, NULL, 0, "/foo/bar", NULL, NULL, "/foo/bar");
	test_parse_pass("/foo?ab=cd", NULL, NULL, NULL, NULL, NULL, 0, "/foo", "ab=cd", NULL, "/foo?ab=cd");
	test_parse_pass("/foo?ab=cd&ef=123", NULL, NULL, NULL, NULL, NULL, 0, "/foo", "ab=cd&ef=123", NULL, "/foo?ab=cd&ef=123");
	test_parse_pass("/foo#/asdf/qwer", NULL, NULL, NULL, NULL, NULL, 0, "/foo", NULL, "/asdf/qwer", "/foo#/asdf/qwer");
	test_parse_pass("/foo?ab#cd", NULL, NULL, NULL, NULL, NULL, 0, "/foo", "ab", "cd", "/foo?ab#cd");
	test_parse_pass("?_", NULL, NULL, NULL, NULL, NULL, 0, NULL, "_", NULL, "?_");
	test_parse_pass("?blah", NULL, NULL, NULL, NULL, NULL, 0, NULL, "blah", NULL, "?blah");
	test_parse_pass("#asdf", NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, "asdf", "#asdf");
	test_parse_pass("#a", NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, "a", "#a");

	// negative tests
	test_parse_fail("");
	test_parse_fail("    ");
	test_parse_fail("?");
	test_parse_fail("#");

	// decode URI components
	test_parse_pass("/foo\%20bar?baz\%20asdf#widget\%20wadget", NULL, NULL, NULL, NULL, NULL, 0, "/foo bar", "baz asdf", "widget wadget",
					"/foo\%20bar?baz\%20asdf#widget\%20wadget");
	test_parse_pass("/%F0%9F%98%80", NULL, NULL, NULL, NULL, NULL, 0, "/😀", NULL, NULL, "/%F0%9F%98%80");
	test_parse_pass("/%f0%9f%99%83%f0%9f%99%83%f0%9f%99%83", NULL, NULL, NULL, NULL, NULL, 0, "/🙃🙃🙃", NULL, NULL,
					"/%F0%9F%99%83%F0%9F%99%83%F0%9F%99%83");

	return 0;
}