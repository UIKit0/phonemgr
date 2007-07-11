
#ifndef _PHONEMGR_OBJECT_H
#define _PHONEMGR_OBJECT_H

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

void phonemgr_object_emit_number_batteries_changed (PhonemgrObject *o, guint num_batteries);
void phonemgr_object_emit_battery_state_changed (PhonemgrObject *o, guint index, guint percentage, gboolean on_ac);

G_END_DECLS

#endif /* !_PHONEMGR_OBJECT_H */
