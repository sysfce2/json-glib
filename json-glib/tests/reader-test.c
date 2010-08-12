#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include <json-glib/json-glib.h>

static const gchar *test_base_array_data =
"[ 0, true, null, \"foo\", 3.14, [ false ], { \"bar\" : 42 } ]";

static const gchar *test_base_object_data =
"{ \"text\" : \"hello, world!\", \"foo\" : \"bar\", \"blah\" : 47 }";

static void
test_base_object (void)
{
  JsonReader *reader = json_reader_new ();
  GError *error = NULL;

  json_reader_load_from_data (reader, test_base_object_data, -1, &error);
  g_assert (error == NULL);

  g_assert (json_reader_is_object (reader));
  g_assert_cmpint (json_reader_count_members (reader), ==, 3);

  g_assert (json_reader_read_member (reader, "foo"));
  g_assert (json_reader_is_value (reader));
  g_assert_cmpstr (json_reader_get_value_string (reader), ==, "bar");
  json_reader_end_member (reader);

  g_assert (!json_reader_read_member (reader, "bar"));
  g_assert (json_reader_get_error (reader) != NULL);
  g_assert_error ((GError *) json_reader_get_error (reader),
                  JSON_READER_ERROR,
                  JSON_READER_ERROR_INVALID_MEMBER);
  json_reader_end_member (reader);
  g_assert (json_reader_get_error (reader) == NULL);

  g_object_unref (reader);
}

static void
test_base_array (void)
{
  JsonReader *reader = json_reader_new ();
  GError *error = NULL;

  json_reader_load_from_data (reader, test_base_array_data, -1, &error);
  g_assert (error == NULL);

  g_assert (json_reader_is_array (reader));
  g_assert_cmpint (json_reader_count_elements (reader), ==, 7);

  json_reader_read_element (reader, 0);
  g_assert (json_reader_is_value (reader));
  g_assert_cmpint (json_reader_get_value_int (reader), ==, 0);
  json_reader_end_element (reader);

  json_reader_read_element (reader, 3);
  g_assert (json_reader_is_value (reader));
  g_assert_cmpstr (json_reader_get_value_string (reader), ==, "foo");
  json_reader_end_element (reader);

  json_reader_read_element (reader, 5);
  g_assert (!json_reader_is_value (reader));
  g_assert (json_reader_is_array (reader));
  json_reader_end_element (reader);

  json_reader_read_element (reader, 6);
  g_assert (json_reader_is_object (reader));

  json_reader_read_member (reader, "bar");
  g_assert (json_reader_is_value (reader));
  g_assert_cmpint (json_reader_get_value_int (reader), ==, 42);
  json_reader_end_member (reader);

  json_reader_end_element (reader);

  g_assert (!json_reader_read_element (reader, 7));
  g_assert_error ((GError *) json_reader_get_error (reader),
                  JSON_READER_ERROR,
                  JSON_READER_ERROR_INVALID_INDEX);
  json_reader_end_element (reader);
  g_assert (json_reader_get_error (reader) == NULL);

  g_object_unref (reader);
}

int
main (int   argc,
      char *argv[])
{
  g_type_init ();
  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

  g_test_add_func ("/reader/base-array", test_base_array);
  g_test_add_func ("/reader/base-object", test_base_object);

  return g_test_run ();
}