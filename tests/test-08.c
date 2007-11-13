#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib-object.h>

#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#define TEST_TYPE_BOXED                 (test_boxed_get_type ())
#define TEST_TYPE_OBJECT                (test_object_get_type ())
#define TEST_OBJECT(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_OBJECT, TestObject))
#define TEST_IS_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_OBJECT))
#define TEST_OBJECT_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_OBJECT, TestObjectClass))
#define TEST_IS_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), TEST_TYPE_OBJECT))
#define TEST_OBJECT_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), TEST_TYPE_OBJECT, TestObjectClass))

typedef struct _TestBoxed               TestBoxed;
typedef struct _TestObject              TestObject;
typedef struct _TestObjectClass         TestObjectClass;

struct _TestBoxed
{
  gint foo;
  gboolean bar;
};

struct _TestObject
{
  GObject parent_instance;

  gint foo;
  gboolean bar;
  gchar *baz;
  TestBoxed blah;
};

struct _TestObjectClass
{
  GObjectClass parent_class;
};

GType test_object_get_type (void);

/*** implementation ***/

static TestBoxed *
test_boxed_copy (const TestBoxed *src)
{
  TestBoxed *copy = g_slice_new (TestBoxed);

  *copy = *src;

  return copy;
}

static void
test_boxed_free (TestBoxed *boxed)
{
  if (G_LIKELY (boxed))
    {
      g_slice_free (TestBoxed, boxed);
    }
}

GType
test_boxed_get_type (void)
{
  static GType b_type = 0;

  if (G_UNLIKELY (b_type == 0))
    b_type = g_boxed_type_register_static ("TestBoxed",
                                           (GBoxedCopyFunc) test_boxed_copy,
                                           (GBoxedFreeFunc) test_boxed_free);

  return b_type;
}

enum
{
  PROP_0,

  PROP_FOO,
  PROP_BAR,
  PROP_BAZ,
  PROP_BLAH
};

static void json_serializable_iface_init (gpointer g_iface);

G_DEFINE_TYPE_WITH_CODE (TestObject, test_object, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE,
                                                json_serializable_iface_init));

static gboolean
test_object_deserialize_property (JsonSerializable *serializable,
                                  const gchar      *name,
                                  GValue           *value,
                                  GParamSpec       *pspec,
                                  JsonNode         *node)
{
  gboolean retval = FALSE;

  return retval;
}

static JsonNode *
test_object_serialize_property (JsonSerializable *serializable,
                                const gchar      *name,
                                const GValue     *value,
                                GParamSpec       *pspec)
{
  JsonNode *retval;

  if (strcmp (name, "blah") == 0)
    {
      TestBoxed *boxed;
      JsonObject *obj;
      JsonNode *val;

      retval = json_node_new (JSON_NODE_OBJECT);
      obj = json_object_new ();
      
      boxed = g_value_get_boxed (value);

      val = json_node_new (JSON_NODE_VALUE);
      json_node_set_int (val, boxed->foo);
      json_object_add_member (obj, "foo", val);

      val = json_node_new (JSON_NODE_VALUE);
      json_node_set_boolean (val, boxed->bar);
      json_object_add_member (obj, "bar", val);

      json_node_take_object (retval, obj);

      test_boxed_free (boxed);
    }
  else
    {
      GValue copy = { 0, };

      retval = json_node_new (JSON_NODE_VALUE);

      g_value_init (&copy, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_value_copy (value, &copy);
      json_node_set_value (retval, &copy);
      g_value_unset (&copy);
    }

  return retval;
}

static void
json_serializable_iface_init (gpointer g_iface)
{
  JsonSerializableIface *iface = g_iface;

  iface->serialize_property = test_object_serialize_property;
  iface->deserialize_property = test_object_deserialize_property;
}

static void
test_object_finalize (GObject *gobject)
{
  g_free (TEST_OBJECT (gobject)->baz);

  G_OBJECT_CLASS (test_object_parent_class)->finalize (gobject);
}

static void
test_object_set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_FOO:
      TEST_OBJECT (gobject)->foo = g_value_get_int (value);
      break;
    case PROP_BAR:
      TEST_OBJECT (gobject)->bar = g_value_get_boolean (value);
      break;
    case PROP_BAZ:
      g_free (TEST_OBJECT (gobject)->baz);
      TEST_OBJECT (gobject)->baz = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
test_object_get_property (GObject    *gobject,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_FOO:
      g_value_set_int (value, TEST_OBJECT (gobject)->foo);
      break;
    case PROP_BAR:
      g_value_set_boolean (value, TEST_OBJECT (gobject)->bar);
      break;
    case PROP_BAZ:
      g_value_set_string (value, TEST_OBJECT (gobject)->baz);
      break;
    case PROP_BLAH:
      g_value_set_boxed (value, &(TEST_OBJECT (gobject)->blah));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
test_object_class_init (TestObjectClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = test_object_set_property;
  gobject_class->get_property = test_object_get_property;
  gobject_class->finalize = test_object_finalize;

  g_object_class_install_property (gobject_class,
                                   PROP_FOO,
                                   g_param_spec_int ("foo", "Foo", "Foo",
                                                     0, G_MAXINT, 42,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_BAR,
                                   g_param_spec_boolean ("bar", "Bar", "Bar",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_BAZ,
                                   g_param_spec_string ("baz", "Baz", "Baz",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_BLAH,
                                   g_param_spec_boxed ("blah", "Blah", "Blah",
                                                       TEST_TYPE_BOXED,
                                                       G_PARAM_READABLE));
}

static void
test_object_init (TestObject *object)
{
  object->foo = 0;
  object->bar = TRUE;
  object->baz = NULL; 

  object->blah.foo = object->foo;
  object->blah.bar = object->bar;
}

static const gchar var_test[] =
"{"
"  \"foo\" : 42,"
"  \"bar\" : false,"
"  \"baz\" : \"Test\""
"}";

int
main (int argc, char *argvp[])
{
  GObject *object;
  GError *error;

  g_type_init ();

  error = NULL;
  object = json_construct_gobject (TEST_TYPE_OBJECT, var_test, -1, &error);
  if (error)
    g_error ("*** Unable to parse buffer: %s\n", error->message);

  g_print ("*** TestObject ***\n"
           " foo: %d\n"
           " bar: %s\n"
           " baz: %s\n",
           TEST_OBJECT (object)->foo,
           TEST_OBJECT (object)->bar ? "<true>" : "<false>",
           TEST_OBJECT (object)->baz);
  
  g_object_unref (object);

  return EXIT_SUCCESS;
}
