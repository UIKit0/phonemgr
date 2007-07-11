
G_BEGIN_DECLS

#include <glib-object.h>

typedef struct _PhonemgrObject PhonemgrObject;
typedef struct _PhonemgrObjectClass PhonemgrObjectClass;

struct _PhonemgrObjectClass {
	GObjectClass  parent_class;
	void (* number_batteries_changed) (PhonemgrObject *o, guint num_batteries);
	void (* battery_state_changed) (PhonemgrObject *o,
					guint battery_index,
					guint percentage,
					gboolean on_ac);
};

struct _PhonemgrObject {
	GObject parent;
};

G_END_DECLS
